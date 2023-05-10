// basic_class_with_mqtt
// ---------------------
//
// This example shows how to use the MQTTClient to:
// * connect to an MQTT brocker
// * publish to a topic
// * subscribe to a topic
// * receive messages from subscribed topics
// * Use an external class to handle the MQTTClient Callbacks 
// * Use an external class to handle the MQTTClient client to publish and subscribe to topics
//

#include <Arduino.h>

// MyClass
#include "MyClass.h"
MyClass myClassObject;

#include <ArduinoJson.h>
#include <MQTTClient.h>
MQTTClient mqttClient;

#ifdef ESP32
  #include <WiFi.h>
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
#endif


// Publish time variables:
unsigned long currentLoopMillis = 0;
unsigned long previousPublishMillis = 0;
unsigned long publishTime = 2000; // milliseconds for publishing to MQTT broker from main loop
const char* mainLoop_topic = "/topic/example/loop";


// Replace with your network credentials
const char* ssid = "WIFI_SSID";
const char* password = "WIFI_PASSWORD";


void setup() {
  Serial.begin(115200);


  // Configure mqttClient with the necessary config with a json object:
  StaticJsonDocument<384> doc;
  JsonObject mqtt = doc.createNestedObject("mqtt");
  mqtt["enabled"] = true;
  mqtt["reconnect_mqtt"] = true;
  mqtt["reconnect_retries"] = 10;
  mqtt["reconnect_time_ms"] = 10000;
  mqtt["server"] = "test.mosquitto.org";
  mqtt["port"] = 1883;
  mqtt["id_name"] = "iot-button";
  mqtt["enable_user_and_pass"] = false;
  mqtt["user_name"] = "userName";
  mqtt["user_password"] = "userPassword";
  mqtt["enable_certificates"] = false;
  mqtt["ca_file"] = "/certs/ca.crt";
  mqtt["cert_file"] = "/certs/cert.der";
  mqtt["key_file"] = "/certs/private.der";
  mqtt["enable_websockets"] = false;
  mqtt["websockets_path"] = "/";
  mqtt["pub_topic"][0] = "/iot-button/feed";

  JsonArray mqtt_sub_topic = mqtt.createNestedArray("sub_topic");
  mqtt_sub_topic.add("/iot-button/topi1");
  mqtt_sub_topic.add("/iot-button/topi2");
  mqtt_sub_topic.add("/iot-button/topi3");
  mqtt["task_stack_size"] = 7168;

  serializeJsonPretty(doc, Serial);
  mqttClient.setConfig(mqtt);

  // And include mainLoop_topic to be subscribe:
  mqttClient.addTopicSub(mainLoop_topic);

  // Set MyClass MQTT client and its observer callbacks:
  myClassObject.setMQTTClient(&mqttClient);

  // Initialize Wifi connection:
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());

  #ifdef ESP32
    // Setup the mqtt client only once for ESP32
    // For ESP8266, the mqtt client is setup  and reconnect in the loop()
    mqttClient.setup();
  #endif

  Serial.println("###  Looping time\n");
}

void loop() {

  currentLoopMillis = millis();

  #ifdef ESP8266
    // The MQTTClient::loop() function must be called periodically to auto-reconnect
    // to the broker if the connection get lost.
    // This is not necessary using ESP32 since the FreeRTOS task runs in parallel
    // and it will reconnect the client automatically.
    mqttClient.loop();
  #endif


  // Main Loop:
  if( mqttClient.connected() && (currentLoopMillis - previousPublishMillis > publishTime)) {
    previousPublishMillis = currentLoopMillis;


    mqttClient.publish(mainLoop_topic,"Message from main loop");
    Serial.printf("Message from main loop published to topic %s\n", mainLoop_topic);

  }

  // MyClass Loop:
  myClassObject.loop();

}
