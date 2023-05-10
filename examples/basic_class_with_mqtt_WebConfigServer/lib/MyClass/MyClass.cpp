#include "MyClass.h"


MyClass::MyClass(void) {
  this->nameConfigObject = "MyClass";
};


MyClass::MyClass(String name) {
  this->nameConfigObject = name;
};

void MyClass::parseWebConfig(JsonObjectConst configObject){
  // WebCOnfigServer pass using this callback the updated 
  // json configuration for this class MyClass.
  // This configObject is the "MyClass" json object from 
  // the data/config/config.json file

  // JsonObject received:
  serializeJsonPretty(configObject, Serial);

  this->enabled = configObject["enabled"] | false;
  this->serverName = configObject["serverName"] | "servenamedefault.com";
  this->port = configObject["port"] | 8888;

  if (configObject["arrayString"].size() > 0)
      for (unsigned int i = 0; i < configObject["arrayString"].size(); i++)
          this->arrayString[i] = configObject["arrayString"][i].as<std::string>();
  else
      this->arrayString[0] = configObject["arrayString"].as<std::string>();

};

void MyClass::loop(void){
  currentMillis = millis();
  if( myClass_mqttClient->connected() && (currentMillis - lastPublishMillis > 1200)) {
    lastPublishMillis = currentMillis;
    
    String topic = this->mqttBaseTopic + "MyClass";
    myClass_mqttClient->publish(topic.c_str(),"Message from MyClass loop");
    Serial.printf("Message from MyClass loop published to topic %s\n", topic.c_str());

  }

};

