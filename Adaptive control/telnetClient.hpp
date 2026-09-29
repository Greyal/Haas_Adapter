#include <iostream>
#include <string>

#include <boost/asio.hpp>

#include "event.hpp"

#pragma once

using boost::asio::ip::tcp;
using namespace std;

enum HaasCodes {
    spindleLoad = 1098,
    alarm = 3000,
    tool = 3026,
    spindleRPM = 3027,
    toolID = 8550, //Same as tool/3026? Check difference 
    feedrate = 62592,
    spindleOverride = 62540,
    feedOverride = 62590,
    rapidOverride = 62591,
    feedRate = 62592
};

class telnetClient
{
public:
    telnetClient(string ip, string port);
    ~telnetClient();

    string getToolNumber();
    event getToolNumberEvent(string component);
    string getLoad();
    event getLoadEvent(string component);
    string getToolSlot();
    event getToolSlotEvent(string component);
    string getPathfeedrate();
    event getPathfeedrateEvent(string component);

    //bool stopMachine();


private:
    const int bufferSize = 512;

    //boost::asio::streambuf buffer; 
    boost::system::error_code error; 

    boost::asio::io_context io_context;
    tcp::socket socket;
    tcp::resolver resolver;

    string read();
    int write(string data);

    string getData(string query);
    event getEvent(string name, string query, string component);
};
