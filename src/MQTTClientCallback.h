#ifndef MQTTClientCallback_H
#define MQTTClientCallback_H

#include <iostream>
#include <string.h>

struct mqtt_client_event_data {
    std::string topic;
    const char* data;
    int data_len;
};

enum mqtt_client_topic_status {
    ANY = -1,
    ERROR = 0,
    SUBSCRIBED,
    UNSUBSCRIBED,
    SUBSCRIPTION_REQUESTED,
};
struct mqtt_client_topic_data {
    std::string topic;
    int qos;
    int subs_msg_id;
    mqtt_client_topic_status subs_status;
};

class MQTTClient;
class MQTTClientCallback {
public:

    virtual void onConnected(MQTTClient* client) {};
    virtual void onDataReceived(MQTTClient* client, const mqtt_client_event_data *data) {};
    virtual void onSubscribed(MQTTClient* client, const mqtt_client_topic_data *topic) {};
};

#endif // MQTTClientCallback_H