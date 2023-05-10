// basic_class_with_mqtt_WebConfigServer
// -------------------------------------
//
// This example shows how to use the MQTTClient to:
// * Connect to an MQTT brocker
// * Publish to a topic
// * Subscribe to a topic
// * Receive messages from subscribed topics
// * Use an external class to handle the MQTTClient Callbacks 
// * Use an external class to handle the MQTTClient client to publish and subscribe to topics
// * Use WebConfigServer to handle MQTTClient and Wifi client reconnection
// * Use WebConfigServer to get device MQTT base topic for publishing
// * Use WebConfigServer to configure the custom MyClass class
//

#include <Arduino.h>

// MyClass
#include "MyClass.h"
// "MyClass" should be the same name as the object used into /data/config/config.json file
// which represent the config objects configurable for this class.
// This constructor can be clled specifying the name of the object into the config.json file
// or the json object name can be provided later on, whenn adding the object into the config object
// using config.addConfig(myClassObject, "MyClass");

// MyClass *MyClass =  new MyClass("MyClass");
MyClass myClassObject;

// WebConfigServer Configuration
#include "WebConfigServer.h"
WebConfigServer config;   // <- global configuration object

#include <MQTTClient.h>
MQTTClient *mqttClient;


// Publish time variables:
unsigned long currentLoopMillis = 0;
unsigned long previousPublishMillis = 0;
unsigned long previousMainLoopMillis = 0;
unsigned long publishTime = 2000; // milliseconds for publishing to MQTT broker from main loop
String mainLoop_topic;


// Websocket functions to publish:
String getLoopTime(){ return String(currentLoopMillis - previousMainLoopMillis);}
String getRSSI(){ return String(WiFi.RSSI());}
String getHeapFree(){ return String((float)GET_FREE_HEAP/1000);}

// Pre Sleep Routine:
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
  
  // Get the MQTTClient object from the WebConfigServer config object:
  mqttClient = config.getMQTTClient();
  // Set alse MQTTClient object into MyClass object:
  myClassObject.setMQTTClient(mqttClient);
  // It can be also used the baste topic created from WebConsifServer:
  myClassObject.setMQTTBaseTopic(config.getDeviceTopic());
  // And include mainLoop_topic to be subscribe:
  mainLoop_topic = config.getDeviceTopic() + "test";
  mqttClient->addTopicSub(mainLoop_topic.c_str());

  // Add dashboard plots in WebConfigServer:
  config.addDashboardObject("heap_free", getHeapFree);
  config.addDashboardObject("loop", getLoopTime);
  config.addDashboardObject("RSSI", getRSSI);

  // Configure the pre sleep routine:
  config.setPreSleepRoutine(callBeforeDeviceOff);

  Serial.println("###  Looping time\n");
}

void loop() {

  currentLoopMillis = millis();

  // WebConfigServer Loop:
  config.loop();

  // MyClass Loop:
  myClassObject.loop();

  // Main Loop:
  if( mqttClient->connected() && (currentLoopMillis - previousPublishMillis > publishTime)) {
    previousPublishMillis = currentLoopMillis;

    mqttClient->publish(mainLoop_topic.c_str(),"Message from main loop");
    Serial.printf("Message from main loop published to topic %s\n", mainLoop_topic.c_str());

  }

  previousMainLoopMillis = currentLoopMillis;
}
