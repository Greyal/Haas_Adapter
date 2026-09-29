#include <string>

#include "telnetClient.hpp"
#include "machineState.hpp"
#include "mtcClient.hpp"

#pragma once

class haasClient {
public:
	void updateStatus();
	string returnStatus();
	string returnStatusChanges();

	haasClient(string IPaddress);
	~haasClient();

private:
	string ip;

	telnetClient* tc;

	machineState currState;
	machineState oldState;
};