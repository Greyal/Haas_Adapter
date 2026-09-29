#include "telnetClient.hpp"

telnetClient::telnetClient(string ip, string port) : io_context(), socket(io_context), resolver(io_context)
{
    cout << "Establishing telnet connection" << endl;
    boost::asio::connect(socket, resolver.resolve(ip, port)); //Need error handling here 
    cout << "Telnet connection established" << endl;
    this->read();
}

telnetClient::~telnetClient() {
    cout << "Closing telnet connection" << endl;
    socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both); //Reinclude error code? 
    socket.close();
    cout << "Telnet connection closed" << endl;
}

string telnetClient::read() {
    char* data = new char[bufferSize];
    memset(data, 0, bufferSize);

    boost::asio::mutable_buffer buffer(data, bufferSize - 1);
    int bytesRead = socket.read_some(buffer, error);
    string s(data);
    delete[] data;
    return s;
}

int telnetClient::write(string data) {
    auto result = boost::asio::write(socket, boost::asio::buffer(data + "\r\n"));
    return result;
}

string telnetClient::getData(string query) {
    //Call and response
    write(query);
    string response = read();

    if (response == ">") { //Ignore any read that's just a '>' 
        response = read();
    }

    //?E responses and responses to mangled queries returned whole
    if (response[0] == '!' || response[0] == '?') {
        return response;
    }

    //Keep everything after the comma 
    //Cuts off the "MACRO, " or "USING TOOL, " or whatever from the response 
    response = response.substr(response.find(',') + 2, response.length());

    //Remove trailing newline characters, carriage returns, and angle brackets
    while (response[response.length() - 1] == '\n' || response[response.length() - 1] == '>' || response[response.length() - 1] == '\r') {
        response.erase(response.length() - 1);
    }

    return response;
}

event telnetClient::getEvent(string name, string query, string component) {
    event ret(name, getData(query),
        boost::posix_time::to_iso_extended_string(boost::posix_time::microsec_clock::universal_time()) + "+00:00", component);
    return ret;
}

string telnetClient::getToolNumber() {
    string toolNum = getData("?Q201");
    return toolNum;
}

event telnetClient::getToolNumberEvent(string component) {
    event ret("toolnumber", getToolNumber(),
        boost::posix_time::to_iso_extended_string(boost::posix_time::microsec_clock::universal_time()) + "+00:00", component);
    return ret;
}

string telnetClient::getLoad() {
    string ret = getData("?Q600 1098");
    ret = ret.substr(0, ret.find('.'));
    return ret;
}

event telnetClient::getLoadEvent(string component) {
    event ret("load", getLoad(),
        boost::posix_time::to_iso_extended_string(boost::posix_time::microsec_clock::universal_time()) + "+00:00", component);
    return ret;
}

string telnetClient::getToolSlot() {
    string ret = getData("?Q600 3026");
    ret = ret.substr(0, ret.find('.'));
    return ret;
}

event telnetClient::getToolSlotEvent(string component) {
    event ret("gcode.toolSlotId", getToolSlot(),
        boost::posix_time::to_iso_extended_string(boost::posix_time::microsec_clock::universal_time()) + "+00:00", component);
    return ret;
}

string telnetClient::getPathfeedrate() {
    string ret = getData("?Q600 62592");
    ret = ret.substr(0, ret.find('.')); 
    return ret;
}

event telnetClient::getPathfeedrateEvent(string component) {
    event ret("pathfeedrate", getPathfeedrate(),
        boost::posix_time::to_iso_extended_string(boost::posix_time::microsec_clock::universal_time()) + "+00:00", component);
    return ret;
}

/*
bool telnetClient::stopMachine() {
    write("?E3000 1");
    string ret = read();
    return ret == "!";
}*/