#include "event.hpp"

#pragma once

using namespace std;

struct machineState {
    event execution;
    vector<event> alarms;
    event ncode;
    event gcodeProgram;
    event programName;
    event toolSlot; //Telnet
    event load; //Telnet
    event partcount;
    event feedrate; //Telnet
    event program;
    event rapidOverride;
    event spindleSpeedActual;
    event spindleOverride;
    event feedOverride;
    event toolNumber; //Telnet 
    event warning;
};