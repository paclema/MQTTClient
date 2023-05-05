#include <Arduino.h>


// Device configurations
unsigned long currentLoopMillis = 0;
unsigned long previousPublishMillis = 0;
unsigned long previousMainLoopMillis = 0;


// WebConfigServer Configuration
#include "WebConfigServer.h"
WebConfigServer config;   // <- global configuration object

#include <MQTTClient.h>
MQTTClient *mqttClient;

// MyClass
#include "MyClass.h"
// "MyClass" should be the same name as the object used into /data/config/config.json file
// which represent the config objects configurable for this class.
// This constructor can be clled specifying the name of the object into the config.json file
// or the json object name can be provided later on, whenn adding the object into the config object
// using config.addConfig(myClassObject, "MyClass");

// MyClass *MyClass =  new MyClass("MyClass");
MyClass myClassObject;


// Websocket functions to publish:
String getLoopTime(){ return String(currentLoopMillis - previousMainLoopMillis);}
String getRSSI(){ return String(WiFi.RSSI());}
String getHeapFree(){ return String((float)GET_FREE_HEAP/1000);}

void callBeforeDeviceOff(){
  Serial.println("Going to sleep!");
};


void setup() {
  Serial.begin(115200);


  // Add MyClass object into WebConfigServer config object so MyClass will receive
  // the new config changes from the config.json file whenever WebConfigServer detects
  // or updates that file. In that case, WebConfigServer will call the MyClass::parseWebConfig 
  // callback method, passing the new (nested, not the whole config.json) "MyClass" JsonObject 
  // with the new configurations.
  config.addConfig(myClassObject, "MyClass");

  config.begin();
  
  mqttClient = config.getMQTTClient();
  myClassObject.setMQTTClient(mqttClient);
  myClassObject.setMQTTBaseTopic(config.getDeviceTopic());

  config.addDashboardObject("heap_free", getHeapFree);
  config.addDashboardObject("loop", getLoopTime);
  config.addDashboardObject("RSSI", getRSSI);

  config.setPreSleepRoutine(callBeforeDeviceOff);

  Serial.println("###  Looping time\n");
}

void loop() {

  currentLoopMillis = millis();

  config.loop();

  myClassObject.loop();

  // Main Loop:
  if( mqttClient->connected() && (currentLoopMillis - previousPublishMillis > 2000)) {
    previousPublishMillis = currentLoopMillis;

    String topic = config.getDeviceTopic() + "/test";
    mqttClient->publish(topic.c_str(),"Message from main loop");

  }

previousMainLoopMillis = currentLoopMillis;
}
