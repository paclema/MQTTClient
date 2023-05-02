#ifndef MQTTClientCallback_H
#define MQTTClientCallback_H

class MQTTClient;

class MQTTClientCallback {
public:
    struct mqtt_client_event_data {
        std::string topic;
        const char* data;
        int data_len;
    };

    virtual void onConnected(MQTTClient* client) {};
    virtual void onDataReceived(MQTTClient* client, const mqtt_client_event_data data) {};
    virtual void onSubscribed(MQTTClient* client) {};
};

#endif // MQTTClientCallback_H