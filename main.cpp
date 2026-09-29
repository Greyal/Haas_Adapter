#include <string>
#include <iostream>
#include <fstream>
#include <chrono>
#include <vector>

#include <boost/json.hpp>
#include <boost/uuid/detail/md5.hpp> //For MD5 hashing
#include <boost/date_time/posix_time/posix_time.hpp> //For ISO time 

#include "MTCclient.hpp"
#include "telnetClient.hpp"
#include "event.hpp"
#include "machineState.hpp"
#include "iotClient.hpp"
#include "haasClient.hpp"

using namespace std;

int main(int argc, char** argv)
{
    try
    {
        ofstream outputFile;
        outputFile.open("HaasTestLog.txt");

        int dataPollingRate = 1000;


        cout << "Creating sender" << endl;
        std::mutex receiveMutex;
        std::condition_variable receiveSignal;
        iotClient sender("a157q8330aj1dg-ats.iot.us-east-2.amazonaws.com", "HAAS-VF2", &receiveMutex, &receiveSignal);
        cout << "Sender created, connecting " << endl;
        sender.connect();
        cout << "Connect succeeded" << endl;
        sender.subscribe("iot/HAS/CNC-1");
        cout << "Subscription complete" << endl;


        haasClient vf2("12");
        cout << vf2.returnStatus() << endl;
        outputFile << vf2.returnStatus() << endl;

        uint64_t prevTime = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count();
        int count = 0;
        while (true) {
            uint64_t currTime = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count();
            if (currTime - prevTime >= dataPollingRate) {
                cout << "Time is " << count << endl;
                vf2.updateStatus();
                cout << vf2.returnStatusChanges() << endl;
                outputFile << vf2.returnStatusChanges() << endl;
                sender.publish(vf2.returnStatusChanges());

                prevTime += dataPollingRate;
                count += 1;
            }
        }

        sender.unsubscribe();
    }
    catch (std::exception const& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        string c;
        cin >> c;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
    
}

