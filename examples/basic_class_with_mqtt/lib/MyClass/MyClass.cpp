#include "MyClass.h"


MyClass::MyClass(void) {
};

void MyClass::loop(void){
  currentMillis = millis();
  if( myClass_mqttClient->connected() && (currentMillis - lastPublishMillis > 1200)) {
    lastPublishMillis = currentMillis;

    myClass_mqttClient->publish(myClass_topic,"Message from MyClass loop");
    Serial.printf("Message from MyClass loop published to topic %s\n", myClass_topic);
  }

};

