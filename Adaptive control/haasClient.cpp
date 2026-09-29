#include "HaasClient.hpp"


haasClient::haasClient(string IPaddress) {
    ip = IPaddress;
    tc = new telnetClient("10.140.80.24", "5051");
    updateStatus();
}

haasClient::~haasClient() {
    delete tc;
}

/*
Proper reporting order: 
Important: Program name, execution, tool number, load 
Unimportant as long as it's after the important stuff: tool slot, ncode, partcount, program, spindlespeed, overrides, pathfeedrate, alarms 
*/

void haasClient::updateStatus() {
    mtcClient MTCclient("10.140.80.24", "8082", "/current");
    MTCclient.write();
    MTCclient.read();

    oldState = currState;

    currState.toolSlot = tc->getToolSlotEvent("1"); //Telnet stuff
    currState.load = tc->getLoadEvent("1");
    currState.feedrate = tc->getPathfeedrateEvent("1");
    currState.toolNumber = tc->getToolNumberEvent("1");
    currState.execution = MTCclient.getEvent("execution", "rstat", "1"); //MTC stuff
    currState.ncode = MTCclient.getNcodeEvent("1");
    currState.program = MTCclient.getEvent("program", "rstat", "1");
    currState.programName = MTCclient.getProgramNameEvent("1");
    currState.partcount = MTCclient.getEvent("partcount", "m30c1", "1");
    //currState.gcodeProgram = MTCclient.getEvent("ncprog");
    currState.rapidOverride = MTCclient.getEvent("rapidoverride", "rovrd", "1");
    currState.spindleSpeedActual = MTCclient.getEvent("spindlespeed", "sspeed", "1");
    currState.spindleOverride = MTCclient.getEvent("spindleoverride", "ssovrd", "1");
    currState.feedOverride = MTCclient.getEvent("feedrateoverride", "fdovrd", "1");
    currState.alarms = MTCclient.getAlarmEventList("1"); 
}

string haasClient::returnStatus() {
    stringstream outputMessage;
    outputMessage << currState.programName.makeJSON() << ", ";
    outputMessage << currState.execution.makeJSON() << ", ";
    outputMessage << currState.toolNumber.makeJSON() << ", ";
    outputMessage << currState.load.makeJSON() << ", ";
    outputMessage << currState.toolSlot.makeJSON() << ", ";
    outputMessage << currState.ncode.makeJSON() << ", ";
    outputMessage << currState.partcount.makeJSON() << ", ";
    outputMessage << currState.program.makeJSON() << ", ";
    outputMessage << currState.spindleSpeedActual.makeJSON() << ", ";
    outputMessage << currState.spindleOverride.makeJSON() << ", ";
    outputMessage << currState.feedOverride.makeJSON() << ", ";
    outputMessage << currState.rapidOverride.makeJSON() << ", ";
    outputMessage << currState.feedrate.makeJSON() << ", ";
    for (int i = 0; i < currState.alarms.size(); i++) {
        outputMessage << currState.alarms[i].makeJSON() << ", ";
    }
    if (outputMessage.str().length() > 0) {
        return "[" + outputMessage.str().substr(0, outputMessage.str().length() - 2) + "]";
    }
    else {
        return "";
    }
}

string haasClient::returnStatusChanges() {
    stringstream outputMessage;
    if (currState.programName != oldState.programName) {
        outputMessage << currState.programName.makeJSON() << ", ";
    }
    if (currState.execution != oldState.execution) {
        outputMessage << currState.programName.makeJSON() << ", ";
        outputMessage << currState.execution.makeJSON() << ", ";
        outputMessage << currState.toolNumber.makeJSON() << ", ";
        outputMessage << currState.load.makeJSON() << ", ";
    }
    if (currState.toolNumber != oldState.toolNumber) {
        outputMessage << currState.toolNumber.makeJSON() << ", ";
    }
    if (currState.load != oldState.load) {
        outputMessage << currState.load.makeJSON() << ", ";
    }
    if (currState.toolSlot != oldState.toolSlot) {
        outputMessage << currState.toolSlot.makeJSON() << ", ";
    }
    if (currState.ncode != oldState.ncode) {
        outputMessage << currState.ncode.makeJSON() << ", ";
    }
    if (currState.partcount != oldState.partcount) {
        outputMessage << currState.partcount.makeJSON() << ", ";
    }
    if (currState.program != oldState.program) {
        outputMessage << currState.program.makeJSON() << ", ";
    }
    if (currState.spindleSpeedActual != oldState.spindleSpeedActual) {
        outputMessage << currState.spindleSpeedActual.makeJSON() << ", ";
    }
    if (currState.spindleOverride != oldState.spindleOverride) {
        outputMessage << currState.spindleOverride.makeJSON() << ", ";
    }
    if (currState.feedOverride != oldState.feedOverride) {
        outputMessage << currState.feedOverride.makeJSON() << ", ";
    }
    if (currState.rapidOverride != oldState.rapidOverride) {
        outputMessage << currState.rapidOverride.makeJSON() << ", ";
    }
    if (currState.feedrate != oldState.feedrate) {
        outputMessage << currState.feedrate.makeJSON() << ", ";
    }
    if (currState.alarms != oldState.alarms) {
        for (int i = 0; i < currState.alarms.size(); i++) {
            outputMessage << currState.alarms[i].makeJSON() << ", ";
        }
    }
    if (outputMessage.str().length() > 0) {
        return "[" + outputMessage.str().substr(0, outputMessage.str().length() - 2) + "]";
    }
    else {
        return "";
    }
}