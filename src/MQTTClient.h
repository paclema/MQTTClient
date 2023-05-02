#ifndef MQTTClient_H
#define MQTTClient_H
#pragma once

#include <Arduino.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

#include <IWebConfig.h>
#include <MQTTClientCallback.h>

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <list>
#include <string.h>

#ifdef ESP32
	#include "freertos/FreeRTOS.h"
	#include "freertos/task.h"
	#include "freertos/semphr.h"
	#include "freertos/queue.h"

	#include "lwip/sockets.h"
	#include "lwip/dns.h"
	#include "lwip/netdb.h"

	#include "esp_wifi.h"
	#include "esp_system.h"
	#include "nvs_flash.h"
	#include "esp_event.h"
	#include <esp_event_base.h>
	#include "esp_netif.h"
	#include "esp_tls.h"
	#include "esp_log.h"
	#include "mqtt_client.h"
#elif defined(ESP8266)
	#include <PubSubClient.h>
	#include <WiFiClientSecure.h>
	#include "WebSocketStreamClient.h"
	#include <ESP8266WiFi.h>
#else
    #error "MQTTClient class only supports ESP32 or ESP8266 targets. Please report an issue at https://github.com/paclema/MQTTClient/issues to check for compatibility with other targets."
#endif


#define MQTT_TOPIC_MAX_SIZE_LIST 10
static const char *TAG = "MQTTClient";


// Possible values for client.state()
typedef int8_t MQTTClientState;
#define MQTT_CONNECTED                		((MQTTClientState)   0)
#define MQTT_CONNECT_BAD_PROTOCOL           ((MQTTClientState)   1)
#define MQTT_CONNECT_BAD_CLIENT_ID          ((MQTTClientState)   2)
#define MQTT_CONNECT_UNAVAILABLE            ((MQTTClientState)   3)
#define MQTT_CONNECT_BAD_CREDENTIALS       	((MQTTClientState)   4)
#define MQTT_CONNECT_UNAUTHORIZED       	((MQTTClientState)   5)

#define MQTT_DISCONNECTED             		((MQTTClientState)  -1)
#define MQTT_CONNECT_FAILED            		((MQTTClientState)  -2)
#define MQTT_CONNECTION_LOST    			((MQTTClientState)  -3)
#define MQTT_CONNECTION_TIMEOUT           	((MQTTClientState)  -4)


typedef enum mqtt_client_topic_status{
    ANY = -1,
    ERROR = 0,
    SUBSCRIBED,
    UNSUBSCRIBED,
	SUBSCRIPTION_REQUESTED,
};

typedef struct mqtt_client_topic_data {
    std::string topic;                      /*!< Topic associated with this event */
    int qos;                            	/*!< qos of the messages associated with this event */
	int subs_msg_id;
	mqtt_client_topic_status subs_status;
};

class MQTTClient : public IWebConfig, private MQTTClientCallback {
public:

	MQTTClient();
	~MQTTClient();

	void setup();
	void addCallback(MQTTClientCallback* callback) { callbacks.push_back(callback); }
	MQTTClient*  *getMQTTClient(void) { return &instance; }

	bool connected(){ return currentState == MQTT_CONNECTED; };
	MQTTClientState state();
	bool isEnabled(void) { return enabled;}
	bool useWebsockets(void) { return enable_websockets; }
	void setMQTTClientId(std::string client_id);

	void addTopicSub(const char* topic, int qos);
	void addTopicSub(const char* topic);
	mqtt_client_topic_data getTopicSub(std::string topicName);
	bool getTopicIsSubscribed(std::string topicName);
  	std::string getBaseTopic(void) { return base_topic_pub; }

	int publish(const char *topic, const char *data, int len, int qos, int retain);
	int publish(const char *topic, const char *data);

	void parseWebConfig(JsonObjectConst configObject);


	#ifdef ESP32
	#elif defined(ESP8266)
	  	void disconnect(void);
		void reconnect(void);
		void loop(void);

		bool getReconnect(void) { return reconnect_mqtt; }
		static void callbackMQTT(char* topic, byte* payload, unsigned int length);
	#endif


private:

	virtual void onConnected(MQTTClient* client) {
		for (MQTTClientCallback* callback : callbacks) {
			callback->onConnected(client);
		}
	}

	virtual void onDataReceived(MQTTClient* client, mqtt_client_event_data data) {
		for (MQTTClientCallback* callback : callbacks) {
			callback->onDataReceived(client, data);
		}
	}

	virtual void onSubscribed(MQTTClient* client) {
		for (MQTTClientCallback* callback : callbacks) {
			callback->onSubscribed(client);
		}
	}

	static MQTTClient* instance;
	#ifdef ESP32
		esp_mqtt_client_handle_t client;

		static void eventHandler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);
		static void log_error_if_nonzero(const char *message, int error_code);
		std::string read_cert_file(const char* filepath);
	#elif defined(ESP8266)
		BearSSL::WiFiClientSecure wifiClientSecure;

		BearSSL::X509List *rootCert;
		BearSSL::X509List *clientCert;
		BearSSL::PrivateKey *clientKey;

		WiFiClient *wifiClient = nullptr;
		WebSocketClient  *wsClient = nullptr;
		WebSocketStreamClient *wsStreamClient = nullptr;
		PubSubClient mqttClient;

		unsigned long previousMqttReconnectionMillis = millis();
		int mqttRetries = 0;

		unsigned long currentLoopMillis = 0;
		unsigned long connectionTime = millis();
	#endif

	bool enabled;
	std::string server;
	int port;
	std::string id_name;
	std::string base_topic_pub;
	bool reconnect_mqtt;
	int mqttMaxRetries;
	unsigned int mqttReconnectionTime;
	bool enable_user_and_pass;
	std::string user_name;
	std::string user_password;
	bool enable_certificates;
	std::string ca_file_path;
	std::string client_cert_file_path;
	std::string client_key_file_path;
	char * client_cert_pem;
	char * client_key_pem;
	char * ca_cert_pem;
	bool enable_websockets;
	std::string websockets_path;
	std::string broker_url;
	std::string pub_topic[MQTT_TOPIC_MAX_SIZE_LIST];
	std::string sub_topic[MQTT_TOPIC_MAX_SIZE_LIST];
	int task_stack_size;

	std::list<MQTTClientCallback*> callbacks;
	std::list<mqtt_client_topic_data> subTopics;

	MQTTClientState currentState = MQTT_DISCONNECTED;

	char* bufOnData = nullptr;
	std::string bufOnDataTopic;
  	size_t bufOnDataSize = 0;
	size_t bufOnDataReceivedSize = 0;


};

#endif // MQTT_CLIENT_H