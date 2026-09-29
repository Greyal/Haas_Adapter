#include <boost/beast.hpp>

#include "event.hpp"

#pragma once

using namespace std;

class mtcClient
{
public:
    mtcClient(string h, string p, string t, int v = 0);
    ~mtcClient();

    void write();
    boost::beast::http::response<boost::beast::http::string_body> read();

    vector<string> readAlarms();
    string readAlarm(int num);
    string readValue(string s);
    string readTime();
    string readProgramName();
    string readNcode();
    event getEvent(string name, string query, string component);
    event getAlarmEvent(int num, string component);
    vector<event> getAlarmEventList(string component);
    event getProgramNameEvent(string component);
    event getNcodeEvent(string component);

private:
    boost::asio::io_context ioc;
    boost::asio::ip::tcp::resolver resolver;
    boost::beast::tcp_stream stream;
    string host;
    string port;
    string target;
    int version;

    boost::beast::http::response<boost::beast::http::string_body> res;
};