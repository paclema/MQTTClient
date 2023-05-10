#ifndef MyClass_H
#define MyClass_H

#include <MQTTClient.h>

#include <string.h>

class MyClass: public MQTTClientCallback {

private:

  MQTTClient * myClass_mqttClient;

  unsigned long currentMillis = millis();
  unsigned long lastPublishMillis = currentMillis;

  const char* myClass_topic = "/topic/example/myClass";



  // MQTTClient Observer callback functions:
  virtual void onConnected(MQTTClient* client) { 
    Serial.println("MyClass receives MQTTClient onConnected callback. The MQTT client is connected to the broker now.");
  };
	virtual void onDataReceived(MQTTClient* client, const mqtt_client_event_data *data) { 
    Serial.printf("[%lu] +++ MyClass receives MQTT message of size %d on topic %s: %.*s\r\n", millis(), data->data_len, data->topic.c_str(), data->data_len, data->data);
  };
  virtual void onSubscribed(MQTTClient* thisClient, const mqtt_client_topic_data *topic){ 
    Serial.printf("MyClass receives new topic subscription status -> Topic[%d]: %s status %d\n", topic->subs_msg_id, topic->topic.c_str(), topic->subs_status);
  };


public:

  MyClass(void);
  void loop(void);

  // MQTTClient Observer callback functions:
  void setMQTTClient(MQTTClient *client){
    // Set MyClass MQTT client and its observer callbacks:
    this->myClass_mqttClient = client;
    this->myClass_mqttClient->addCallback(this);
    // Subscribe to topic example to receive published messages
    this->myClass_mqttClient->addTopicSub(myClass_topic);
  }

};
#endif
