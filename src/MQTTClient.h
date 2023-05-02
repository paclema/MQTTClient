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
	bool isEnabled(void) { return enabled;}

	void addCallback(MQTTClientCallback* callback) {
		callbacks.push_back(callback);
	}

	void parseWebConfig(JsonObjectConst configObject);

	int publish(const char *topic, const char *data, int len, int qos, int retain){
		return esp_mqtt_client_publish(client, topic, data, len, qos, retain);
	}

	int publish(const char *topic, const char *data){
		ESP_LOGI(TAG, "MQTT TX -> topic: %s, message: %s", topic, data);
		return publish(topic, data, 0, 0, 0);
	}

	void addTopicSub(const char* topic, int qos){
		mqtt_client_topic_data newTopic = {
			.topic = topic,
			.qos = qos,
			.subs_msg_id = -1,
			.subs_status = ANY
		};

		if (currentState == MQTT_CONNECTED){
			newTopic.subs_msg_id = esp_mqtt_client_subscribe(client, newTopic.topic.c_str(), newTopic.qos);
		};

		subTopics.push_back(newTopic);

	}

	mqtt_client_topic_data getTopicSub(std::string topicName){
		for (mqtt_client_topic_data t : subTopics) {
			if (t.topic == topicName) return t;
		}
	}

	bool getTopicIsSubscribed(std::string topicName){
		for (mqtt_client_topic_data t : subTopics) {
			if (t.topic == topicName) {
				// ESP_LOGW(TAG, "FOUND topic %s with status=%d", t.topic.c_str(), t.subs_status);
				if (t.subs_status == SUBSCRIBED) return true;
				else return false;
			}
		}
	}

	void addTopicSub(const char* topic){
		addTopicSub(topic, 0);
	}

	void setMQTTClientId(std::string client_id) {
		id_name = client_id;
		StaticJsonDocument<192> docSave;
		docSave["id_name"] = this->id_name;
		IWebConfig::saveWebConfig(docSave.as<JsonObject>());
	}

	bool connected(){
		if (currentState == MQTT_CONNECTED)
			return true;
		else
			return false;
	}
	int state(){
		//TODO: create state machine with real client status
		// using esp_mqtt_error_codes_t arriving on the 
		// within MQTT_EVENT_ERROR type event MQTTClient::eventHandler
		return currentState;
	}


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
	esp_mqtt_client_handle_t client;

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


	static void eventHandler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);
	static void log_error_if_nonzero(const char *message, int error_code);
	std::string read_cert_file(const char* filepath);


};

#endif // MQTT_CLIENT_H