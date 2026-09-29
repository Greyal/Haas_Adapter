#include <string>
#include <queue>
#include <mutex>

#include <aws/iot/Mqtt5Client.h>

#pragma once

using namespace Aws;

class iotClient {
public:
	std::shared_ptr<Crt::Mqtt5::Mqtt5Client> client;
	std::mutex* receiveMutex;
	std::condition_variable* receiveSignal;
	uint32_t receivedCount = 0;
	uint32_t publishedCount = 0;

	std::promise<bool> connectionPromise;
	std::promise<void> stoppedPromise;
	std::promise<void> disconnectPromise;
	std::promise<bool> subscribeSuccess;

	std::string topic;
	std::queue<std::string> receivedQueue;

	iotClient(std::string endpoint, std::string clientId, std::mutex* m, std::condition_variable* c);
	~iotClient();

	int connect();
	int subscribe(std::string topic);
	int publish(std::string message);
	int unsubscribe();
};