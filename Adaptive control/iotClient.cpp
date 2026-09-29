#include "iotClient.hpp"

#include <string>
#include <iostream>

#include <aws/crt/mqtt/mqtt5packets.h>
#include <aws/iot/Mqtt5Client.h>
#include <aws/crt/Api.h>

using namespace Aws;

iotClient::iotClient(std::string endpoint, std::string clientId, std::mutex* m, std::condition_variable* c) {
    receiveMutex = m;
    receiveSignal = c;

    //Crt::ApiHandle apiHandle;
    //apiHandle.InitializeLogging(Crt::LogLevel::Debug, "AWSdebug.log");

    aws_common_library_init(aws_default_allocator());
    aws_io_library_init(aws_default_allocator());

    Aws::Iot::Mqtt5ClientBuilder* builder = Iot::Mqtt5ClientBuilder::NewMqtt5ClientBuilderWithMtlsFromPath(
        endpoint.c_str(), //endpoint
        "certificate.pem.crt", //certificate
        "private.pem.key"); //key

    if (builder == nullptr) {
        std::cout << "MQTT5 client factory creation error " << std::endl; //Can happen when cert or key are missing
        return;
    }

    // Setup connection options
    std::shared_ptr<Crt::Mqtt5::ConnectPacket> connectOptions = std::make_shared<Crt::Mqtt5::ConnectPacket>();
    connectOptions->WithClientId(clientId.c_str()); //Client ID

    builder->WithConnectOptions(connectOptions);
    builder->WithPort(8883); 
    builder->WithCertificateAuthority("AmazonRootCA1.pem");

    // Setup lifecycle callbacks
    builder->WithClientConnectionSuccessCallback(
        [this](const Crt::Mqtt5::OnConnectionSuccessEventData& eventData) {
            fprintf(
                stdout, "Mqtt5 Client connection succeed, clientid: %s.\n", eventData.negotiatedSettings->getClientId().c_str());
           connectionPromise.set_value(true);
        });
    builder->WithClientConnectionFailureCallback([this](
        const Crt::Mqtt5::OnConnectionFailureEventData& eventData) {
            std::cout << "Error code " << eventData.errorCode << std::endl;
            fprintf(stdout, "Mqtt5 Client connection failed with error: %s.\n", aws_error_debug_str(eventData.errorCode));
            connectionPromise.set_value(false);
        });
    builder->WithClientStoppedCallback([this](const Crt::Mqtt5::OnStoppedEventData&) {
        fprintf(stdout, "Mqtt5 Client stopped.\n");
        stoppedPromise.set_value();
        });
    builder->WithClientAttemptingConnectCallback([](const Crt::Mqtt5::OnAttemptingConnectEventData&) {
        fprintf(stdout, "Mqtt5 Client attempting connection...\n");
        });
    builder->WithClientDisconnectionCallback([this](const Crt::Mqtt5::OnDisconnectionEventData& eventData) {
        fprintf(stdout, "Mqtt5 Client disconnection with reason: %s.\n", aws_error_debug_str(eventData.errorCode));
        disconnectPromise.set_value();
        });

    // This is invoked upon the receipt of a Publish on a subscribed topic.
    builder->WithPublishReceivedCallback([this](const Crt::Mqtt5::PublishReceivedEventData& eventData) {
            if (eventData.publishPacket == nullptr)
                return;

            std::lock_guard<std::mutex> lock(*receiveMutex);
            ++receivedCount;
            //fprintf(stdout, "Publish received on topic %s:", eventData.publishPacket->getTopic().c_str());
            //fwrite(eventData.publishPacket->getPayload().ptr, 1, eventData.publishPacket->getPayload().len, stdout);
            //fprintf(stdout, "\n");
            
            Crt::StringView s = Aws::Crt::ByteCursorToStringView(eventData.publishPacket->getPayload());
            char* message = new char[eventData.publishPacket->getPayload().len];
            memcpy(message, eventData.publishPacket->getPayload().ptr, eventData.publishPacket->getPayload().len);
            std::string m(message);
            receivedQueue.push(m);
            delete[] message;
            std::cout << m << std::endl;
            for (Crt::Mqtt5::UserProperty prop : eventData.publishPacket->getUserProperties())
            {
                fprintf(stdout, "\twith UserProperty:(%s,%s)\n", prop.getName().c_str(), prop.getValue().c_str());
            }
            receiveSignal->notify_all();
        });

    std::cout << "Creating client" << std::endl;
    // Create Mqtt5Client
    client = builder->Build();
    std::cout << "Client created" << std::endl;
    // Clean up the builder
    delete builder;

    if (client == nullptr)
    {
        //fprintf(stdout, "Failed to Init Mqtt5Client with error code %d: %s", LastError(), ErrorDebugString(LastError()));
        std::cout << "Builder failed" << std::endl;
    }
    return;
}


iotClient::~iotClient() {
    //delete receiveMutex;

    //aws_io_library_clean_up();
    //aws_common_library_clean_up();

    ~*client;
}

int iotClient::connect() {
    // Start mqtt5 connection session
    if (client->Start())
    {
        std::cout << "Client started " << std::endl;
        if (connectionPromise.get_future().get() == false)
        {
            return -1;
        }

        std::cout << "Connection worked" << std::endl;
        return 0;
    }
}

int iotClient::subscribe(std::string topic) {
    this->topic = topic;

    std::promise<bool>* ptrToSubscribeSuccess = &subscribeSuccess;
    auto onSubAck = [&ptrToSubscribeSuccess](int error_code, std::shared_ptr<Crt::Mqtt5::SubAckPacket> suback) {
        if (error_code != 0)
        {
            fprintf(
                stdout,
                "MQTT5 Client Subscription failed with error code: (%d)%s\n",
                error_code,
                aws_error_debug_str(error_code));
            ptrToSubscribeSuccess->set_value(false);
        }
        if (suback != nullptr)
        {
            for (Crt::Mqtt5::SubAckReasonCode reasonCode : suback->getReasonCodes())
            {
                if (reasonCode > Crt::Mqtt5::SubAckReasonCode::AWS_MQTT5_SARC_UNSPECIFIED_ERROR)
                {
                    fprintf(
                        stdout,
                        "MQTT5 Client Subscription failed with server error code: (%d)%s\n",
                        reasonCode,
                        suback->getReasonString()->c_str());
                    ptrToSubscribeSuccess->set_value(false);
                    return;
                }
            }
        }
        ptrToSubscribeSuccess->set_value(true);
        };

    Crt::Mqtt5::Subscription sub1(topic.c_str(), Crt::Mqtt5::QOS::AWS_MQTT5_QOS_AT_LEAST_ONCE);
    sub1.WithNoLocal(false);
    std::shared_ptr<Crt::Mqtt5::SubscribePacket> subPacket = std::make_shared<Crt::Mqtt5::SubscribePacket>();
    subPacket->WithSubscription(std::move(sub1));

    if (client->Subscribe(subPacket, onSubAck))
    {
        // Waiting for subscription completed.
        if (subscribeSuccess.get_future().get() == true)
        {
            fprintf(stdout, "Subscription Success.\n");
        }
        else
        {
            fprintf(stdout, "Subscription failed.\n");
        }
    }
    else
    {
        fprintf(stdout, "Subscribe operation failed on client.\n");
    }
}

int iotClient::publish(std::string message) {
    //Setup publish completion callback. The callback will get triggered when the publish completes (when
    //the client received the PubAck from the server).

    auto onPublishComplete = [](int, std::shared_ptr<Aws::Crt::Mqtt5::PublishResult> result) {
        if (!result->wasSuccessful())
        {
            fprintf(stdout, "Publish failed with error_code: %d", result->getErrorCode());
        }
        else if (result != nullptr)
        {
            std::shared_ptr<Crt::Mqtt5::PubAckPacket> puback =
                std::dynamic_pointer_cast<Crt::Mqtt5::PubAckPacket>(result->getAck());
            if (puback->getReasonCode() == 0)
            {
                fprintf(stdout, "Publish Succeed.\n");
            }
            else
            {
                fprintf(
                    stdout,
                    "PubACK reason code: %d : %s\n",
                    puback->getReasonCode(),
                    puback->getReasonString()->c_str());
            }
        };
    };

    Crt::String AWSstringMessage = message.c_str();
    Crt::ByteCursor payload = Aws::Crt::ByteCursorFromString(AWSstringMessage);
    //Do not simplify this with ByteCursorFromString(message.c_str()), causes weird message garbling 

    std::shared_ptr<Crt::Mqtt5::PublishPacket> publish = std::make_shared<Crt::Mqtt5::PublishPacket>(
        topic.c_str(), payload, Crt::Mqtt5::QOS::AWS_MQTT5_QOS_AT_LEAST_ONCE);
    if (client->Publish(publish, onPublishComplete))
    {
        ++publishedCount;
    }

    {
        std::unique_lock<std::mutex> receivedLock(*receiveMutex);
        receiveSignal->wait(receivedLock, [&] { return receivedCount >= publishedCount; });
    }
}

int iotClient::unsubscribe()
{
    // Unsubscribe from the topic.
    std::promise<void> unsubscribeFinishedPromise;
    std::shared_ptr<Crt::Mqtt5::UnsubscribePacket> unsub = std::make_shared<Crt::Mqtt5::UnsubscribePacket>();
    unsub->WithTopicFilter(topic.c_str());
    if (!client->Unsubscribe(unsub, [&](int, std::shared_ptr<Crt::Mqtt5::UnSubAckPacket>) {
        unsubscribeFinishedPromise.set_value();
        }))
    {
        fprintf(stdout, "Unsubscription failed.\n");
        return -1;
    }
    unsubscribeFinishedPromise.get_future().wait();

    return 0;
}

