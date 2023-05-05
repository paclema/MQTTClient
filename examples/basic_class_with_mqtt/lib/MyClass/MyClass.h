#ifndef MyClass_H
#define MyClass_H

#include "IWebConfig.h"
#include <MQTTClient.h>

#include <string.h>

class MyClass: 
  public IWebConfig,
  public MQTTClientCallback {

private:

  MQTTClient * myClass_mqttClient;  
  String mqttBaseTopic = "/";

  unsigned long currentMillis = millis();
  unsigned long lastPublishMillis = currentMillis;

  bool enabled = false;
  std::string serverName;
  int port;
  std::string arrayString[4];

  // MQTTClient Observer callback functions:
  virtual void onConnected(MQTTClient* client) { 
    Serial.println("MyClass receives MQTTClient onConnected callback. The client is connected to the broker now.");
    };
	virtual void onDataReceived(MQTTClient* client, const mqtt_client_event_data *data) { 
    Serial.printf("[%lu] +++ MyClass receives MQTT message of size %d on topic %s: %s\n", millis(), data->data_len, data->topic.c_str(), data->data);
    };
  virtual void onSubscribed(MQTTClient* thisClient, const mqtt_client_topic_data *topic){ 
    Serial.printf("MyClass receives new topic subscription status -> Topic[%d]: %s status %d\n", topic->subs_msg_id, topic->topic.c_str(), topic->subs_status);
  };

  // IWebConfig callback functions:
  void parseWebConfig(JsonObjectConst configObject);

public:

  MyClass(void);
  MyClass(String name);

  void loop(void);
  void setMQTTBaseTopic(String topic){ this->mqttBaseTopic = topic; }

  // MQTTClient Observer callback functions:
  void setMQTTClient(MQTTClient *client){
    // Set MyClass MQTT client and its observer callbacks:
    this->myClass_mqttClient = client;
    this->myClass_mqttClient->addCallback(this);
  }

};
#endif
