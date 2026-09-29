#include "mtcClient.hpp"

#include <boost/beast.hpp>

#include "event.hpp"

#pragma once

using namespace std;

mtcClient::mtcClient(string h, string p, string t, int v) : resolver(ioc), stream(ioc)
{
    //cout << "Establishing MTC connection" << endl;
    host = h;
    port = p;
    target = t;
    version = v;

    // Look up the domain name
    auto const results = resolver.resolve(host, port);

    // Make the connection on the IP address we get from a lookup
    stream.connect(results);

    //cout << "MTC connection established" << endl;
}

mtcClient::~mtcClient() {
    // Gracefully close the socket
    boost::beast::error_code ec;
    stream.socket().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);

    // not_connected happens sometimes so don't bother reporting it.
    if (ec && ec != boost::beast::errc::not_connected)
        throw boost::beast::system_error{ ec };
}

void mtcClient::write() {
    // Set up an HTTP GET request message
    boost::beast::http::request<boost::beast::http::string_body> req{boost::beast::http::verb::get, target, version};
    req.set(boost::beast::http::field::host, host);
    req.set(boost::beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);

    // Send the HTTP request to the remote host
    boost::beast::http::write(stream, req);
}

boost::beast::http::response<boost::beast::http::string_body> mtcClient::read() {
    boost::beast::flat_buffer buffer;
    boost::beast::http::read(stream, buffer, res);
    return res;
}


vector<string> mtcClient::readAlarms() {
    vector<string> ret;
    int field = res.body().find("dataItemId=\"aalarms\""); //Find aalarms 
    if (field == -1) { //Field not found, problem 
        cout << "ERROR: aalarms NOT FOUND" << endl;
    }
    while (res.body().find("alarmNumber", field) != -1) { //Add each alarm to vector 
        field = res.body().find("alarmNumber", field); //Find the next alarm
        field = res.body().find('>', field); //Find the data after >
        ret.push_back(res.body().substr(field + 1, res.body().find('<', field) - field - 1)); //Add everything between > and <
    }
    if (ret.empty()) { //No alarms
        ret.push_back("0");
    }
    return ret;
}

string mtcClient::readAlarm(int alarmCount) {
    int field = res.body().find("dataItemId=\"aalarms\""); //Find aalarms 
    if (field == -1) {
        return "ERROR: aalarms NOT FOUND";
    }
    if (readValue("aalarms") == "NO ACTIVE ALARMS") {
        return "0";
    }
    for (int i = 0; i <= alarmCount; i++) {
        field = res.body().find("alarmNumber", field); //Find the list of alarms
        if (field == -1) {
            cout << "Error in alarm count" << endl;
            return ""; //Returns an empty string if there are no more alarms 
        }
        field++;
    }
    int dataStart = res.body().find('>', field); //Find the data after >
    int dataEnd = res.body().find('<', dataStart);
    return res.body().substr(dataStart + 1, dataEnd - dataStart - 1); //Return everything between > and <
}

/*string mtcClient::readAlarmTime(string s) {
    if (s == "0") {
        return readTime(); //No alarms, just return timestamp for entire aalarms field 
    }
    int field = res.body().find(s); //Find alarm in question
    if (field == -1) { //Field not found, problem 
        cout << "ERROR: aalarms NOT FOUND" << endl;
    }
    field = res.body().find("timestamp=", field);
    return res.body().substr(field + 11, 23);
}*/

string mtcClient::readValue(string s) {
    s = "dataItemId=\"" + s + "\""; //Add string context 
    int field = res.body().find(s); //Find the desired field
    if (field == -1) {
        return "ERROR: " + s + " NOT FOUND";
    }
    field = res.body().find('>', field); //Find the start of the data in the field 
    string data = res.body().substr(field + 1, res.body().find('<', field) - field - 1); //Get a string of the data 
    return data;
}

string mtcClient::readTime() {/*
    int field = res.body().find("creationTime="); //Find the desired field
    if (field == -1) {
        return "ERROR: TIMESTAMP NOT FOUND";
    }
    int dataStart = res.body().find('"', field); //Find the start of the timestamp 
    dataStart += 1;  
    int dataEnd = res.body().find('"', dataStart + 2); //Find the end of the timestamp 
    if (dataStart == -1 || dataEnd == -1) {
        return "ERROR IN READFIELD FINDING DATA FOR TIMESTAMP";
    }
    string time = res.body().substr(dataStart, dataEnd - dataStart - 1); //Get a string of the timestamp 
    time += "+00:00"; */
    string time = boost::posix_time::to_iso_extended_string(boost::posix_time::microsec_clock::universal_time()) + "+00:00";
    return time;
}

string mtcClient::readProgramName() { //Fifteenth comma-separated value in addresscodes 
    string codes = readValue("addresscodes");
    string codesArray[26];
    for (int i = 0; i < 26; i++) {
        codesArray[i] = codes.substr(0, codes.find(',')); //Separate data before first comma in codes
        codes = codes.substr(codes.find(',') + 1, codes.length()); //Remove through first comma in codes
    }
    codesArray[25] = codes; //Last code doesn't contain a comma, so prev method wouldn't work
    return codesArray[14];
}

string mtcClient::readNcode() { //Fourteenth value in addresscodes
    string codes = readValue("addresscodes");
    string codesArray[26];
    for (int i = 0; i < 26; i++) {
        codesArray[i] = codes.substr(0, codes.find(',')); //Separate data before first comma in codes
        codes = codes.substr(codes.find(',') + 1, codes.length()); //Remove through first comma in codes
    }
    codesArray[25] = codes; //Last code doesn't contain a comma, so prev method wouldn't work
    return 'N' + codesArray[13];
}

//Sixteenth addresscodes value is current dwell? 

event mtcClient::getEvent(string name, string query, string component) {
    event ret(name, readValue(query), readTime(), component);
    return ret;
}

event mtcClient::getAlarmEvent(int num, string component) {
    event ret("fault", readAlarm(num), readTime(), component);
    return ret;
}

vector<event> mtcClient::getAlarmEventList(string component) {
    vector<string> alarms = readAlarms();
    vector<event> retVec;
    for (int i = 0; i < alarms.size(); i++) {
        retVec.push_back(getAlarmEvent(i, component));
    }
    return retVec;
}

event mtcClient::getProgramNameEvent(string component) { 
    event ret("gcode.programname", readProgramName(), readTime(), component);
    return ret;
}

event mtcClient::getNcodeEvent(string component) { 
    event ret("gcode.ncode", readNcode(), readTime(), component);
    return ret;
}