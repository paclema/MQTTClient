#include "MQTTClient.h"


MQTTClient::MQTTClient(){
}

MQTTClient::~MQTTClient() {
    // TODO: chek for deleting certificate buffers:
    // delete(client_cert_pem);

    if (bufOnData != nullptr) {
        delete[] bufOnData;
    }
}

#ifdef ESP32
void MQTTClient::setup() {

    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("MQTT_CLIENT", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_BASE", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT", ESP_LOG_VERBOSE);
    esp_log_level_set("OUTBOX", ESP_LOG_VERBOSE);

    // ESP_ERROR_CHECK(nvs_flash_init());
    // ESP_ERROR_CHECK(esp_netif_init());
    // ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    // ESP_ERROR_CHECK(example_connect());

   
    // We can include the CA to the global store and use .use_global_ca_store = true, 
    // for the mqtt configuration. In that case, we can avoid to provide .cert_pem for mqtt config.
    // This way the CA certificate will be shared among other possible connections.

    // Another way is to use the CA as the server certificate with .cert_pem variable
    // for the mqtt configuration. The counter side is that more memory will be booked 
    // when using multiple connections.

    // It can be also used the server certificate obtained with:
    // openssl s_client -showcerts -connect dev.yourserver.io:1443 < /dev/null 2> /dev/null | openssl x509 -outform PEM > yourserver.pem 
    // but it will expire faster than the CA certificate.

    if(this->enable_certificates){
        esp_err_t errca = esp_tls_init_global_ca_store();
        if (errca != ESP_OK) ESP_LOGE(TAG, "esp_tls_init_global_ca_store error : %d", errca);


        std::string caCertFileStr = read_cert_file(ca_file_path.c_str());
        errca = esp_tls_set_global_ca_store( (const unsigned char *) caCertFileStr.c_str(),
                                            (unsigned int)caCertFileStr.length() + 1);
        if (errca != ESP_OK) ESP_LOGE(TAG, "esp_tls_set_global_ca_store error : %d", errca);


        std::string clientCertFileStr = read_cert_file(client_cert_file_path.c_str());
        client_cert_pem = new(char[clientCertFileStr.length() + 1]);
        strcpy((char *)client_cert_pem, (const char *)clientCertFileStr.c_str());

        std::string clientKeyFileStr = read_cert_file(client_key_file_path.c_str());
        client_key_pem = new(char[clientKeyFileStr.length() + 1]);
        strcpy((char *)client_key_pem, (const char *)clientKeyFileStr.c_str());
        
    }
    
    std::string url_prefix;
    std::string url_sufix;
    if (enable_websockets) {
        url_prefix = "wss://";
        url_sufix = websockets_path;
    }
    else url_prefix = "mqtt://";

    broker_url = url_prefix + server + ":" + std::to_string(port) + url_sufix;
    ESP_LOGI(TAG, "Connecting to broker: %s", broker_url.c_str());
    
    const esp_mqtt_client_config_t mqtt_cfg = {
        .uri = broker_url.c_str(),

        .set_null_client_id = false,
        .client_id = id_name.c_str(),

        .task_stack = task_stack_size,

        .client_cert_pem = this->enable_certificates ? (const char *)client_cert_pem : NULL,
        .client_key_pem = this->enable_certificates ? (const char *)client_key_pem : NULL,
        .use_global_ca_store = this->enable_certificates

    };


    // Full configuration example:
    //  const esp_mqtt_client_config_t mqtt_cfg = {
    //     // .uri = (const char *)"wss://dev.server.io:1443/api/controldevice",
    //     .host = (const char *)"dev.server.io",
    //     .port = (uint32_t)1443,
    //     .set_null_client_id = false,
    //     .client_id = (const char *)"3568872",
    //     // .cert_pem = (const char *)read_cert_file("/certs/api/ca.cert.pem").c_str(),
    //     // .client_cert_pem = (const char *)read_cert_file("/certs/api/client.cert.pem").c_str(),
    //     // .client_key_pem = (const char *)read_cert_file("/certs/api/private.key.pem").c_str(),
    //     // .cert_pem = (const char *)server_cert_pem2,
    //     // .cert_len = (size_t)strlen(server_cert_pem2),
    //     .client_cert_pem = (const char *)client_cert_pem,
    //     // .client_cert_len = (size_t)strlen(client_cert_pem),
    //     .client_key_pem = (const char *)client_key_pem,
    //     // .client_key_len = (size_t)strlen(client_key_pem),
    //     .transport = (esp_mqtt_transport_t)MQTT_TRANSPORT_OVER_WSS,
    //     .use_global_ca_store = true,
    //     .skip_cert_common_name_check = false,
    //     .use_secure_element = false,
    //     .path = (const char *)"api/controldevice",
    // };
    

    ESP_LOGW(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    client = esp_mqtt_client_init(&mqtt_cfg);
    /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
    esp_mqtt_client_register_event(client, (esp_mqtt_event_id_t)ESP_EVENT_ANY_ID, MQTTClient::eventHandler, this);
    esp_mqtt_client_start(client);


    //TODO: When certificate buffer is not used anymore, delete it:
    // delete(client_cert_pem);


}

void MQTTClient::eventHandler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    MQTTClient* thisClient = (MQTTClient*)handler_args;
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    
    if(event->total_data_len == (event->data_len + event->current_data_offset)){
        ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
        #ifdef ARDUINO_LOOP_STACK_SIZE
        ESP_LOGI(TAG, "Free Heap: %d of %d Free PSRAM: %d of %d", ESP.getFreeHeap(), ESP.getHeapSize(), ESP.getFreePsram(), ESP.getPsramSize());
        ESP_LOGI(TAG, "Free Arduino loop Stack: %d/%d", uxTaskGetStackHighWaterMark(NULL), ARDUINO_LOOP_STACK_SIZE);
        #endif
    }

    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGW(TAG, "MQTT_EVENT_CONNECTED");
        thisClient->currentState = MQTT_CONNECTED;
        // msg_id = esp_mqtt_client_subscribe(client, "/topic/qos0", 0);
        // ESP_LOGW(TAG, "sent subscribe successful, msg_id=%d", msg_id);

        // msg_id = esp_mqtt_client_subscribe(client, "/topic/qos1", 1);
        // ESP_LOGW(TAG, "sent subscribe successful, msg_id=%d", msg_id);

        // msg_id = esp_mqtt_client_unsubscribe(client, "/topic/qos1");
        // ESP_LOGW(TAG, "sent unsubscribe successful, msg_id=%d", msg_id);

        // msg_id = esp_mqtt_client_subscribe(client, "#", 0);
        // ESP_LOGW(TAG, "sent subscribe successful, msg_id=%d", msg_id);

        // msg_id = esp_mqtt_client_subscribe(client, "3568872/#", 0);
        // ESP_LOGW(TAG, "sent subscribe successful, msg_id=%d", msg_id);


        for (mqtt_client_topic_data& t : thisClient->subTopics) {
            t.subs_msg_id = esp_mqtt_client_subscribe(client, t.topic.c_str(), t.qos);
            t.subs_status = SUBSCRIPTION_REQUESTED;
            ESP_LOGD(TAG, "Topic[%d]: %s status to %d", t.subs_msg_id, t.topic.c_str(), t.subs_status);
        }

        thisClient->onConnected(thisClient);

        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGW(TAG, "MQTT_EVENT_DISCONNECTED");
        thisClient->currentState = MQTT_DISCONNECTED;
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGW(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        for (mqtt_client_topic_data& t : thisClient->subTopics) {
            if(event->msg_id == t.subs_msg_id){
                t.subs_status = SUBSCRIBED;
                ESP_LOGD(TAG, "Topic[%d]: %s status to %d", t.subs_msg_id, t.topic.c_str(), t.subs_status);
                thisClient->onSubscribed(thisClient, &t);
                break;
            }
        }
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGW(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        for (mqtt_client_topic_data& t : thisClient->subTopics) {
            if(event->msg_id == t.subs_msg_id){
                t.subs_status = UNSUBSCRIBED;
                ESP_LOGD(TAG, "Topic[%d]: %s status to %d", t.subs_msg_id, t.topic.c_str(), t.subs_status);
                break;
            }
        }
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGW(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA: { 
            // ESP_LOGW(TAG, "MQTT_EVENT_DATA");
            // printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
            // printf("DATA=%.*s\r\n", event->data_len, event->data);

            // Verify that the data is not truncated among different MQTT_EVENT_DATA events.
            // In case the data is trunckated, receive all data before calling the onData() observer callback:

            if (event->total_data_len != event->data_len) {
                if(!event->current_data_offset) {
                    ESP_LOGW(TAG, "MQTT_EVENT_DATA - DATA CHUNKED START");
                    ESP_LOGW(TAG, "TOPIC=%.*s", event->topic_len, event->topic);
                    thisClient->bufOnDataTopic = std::string(event->topic, event->topic_len);
                    if (thisClient->bufOnData) {
                        delete[] thisClient->bufOnData;
                        thisClient->bufOnData = nullptr;
                    }
                    thisClient->bufOnDataSize = event->total_data_len;
                    thisClient->bufOnData = new char[thisClient->bufOnDataSize];

                    memcpy(thisClient->bufOnData + thisClient->bufOnDataReceivedSize, event->data, event->data_len);
                    thisClient->bufOnDataReceivedSize += event->data_len;

                } else if(event->total_data_len == event->data_len + event->current_data_offset){
                    ESP_LOGW(TAG, "MQTT_EVENT_DATA - DATA CHUNKED FINISHED");
                    memcpy(thisClient->bufOnData + thisClient->bufOnDataReceivedSize, event->data, event->data_len);
                    thisClient->bufOnDataReceivedSize += event->data_len;
                    ESP_LOGD(TAG, "MQTT_EVENT_DATA, data chunk length received %d. offset %d of %d total data length ", event->data_len, event->current_data_offset, event->total_data_len);
                    
                    mqtt_client_event_data *data = new mqtt_client_event_data{
                        .topic = thisClient->bufOnDataTopic,
                        .data = thisClient->bufOnData,
                        .data_len = thisClient->bufOnDataSize,
                    };

                    thisClient->onDataReceived(thisClient, data);

                    // Clean up buffer
                    delete[] thisClient->bufOnData;
                    thisClient->bufOnData = nullptr;
                    thisClient->bufOnDataSize = 0;
                    thisClient->bufOnDataReceivedSize = 0;
                    delete data;

                } else {
                    ESP_LOGW(TAG, "MQTT_EVENT_DATA - DATA CHUNKED NEXT");
                    memcpy(thisClient->bufOnData + thisClient->bufOnDataReceivedSize, event->data, event->data_len);
                    thisClient->bufOnDataReceivedSize += event->data_len;
                }
            } else {
                ESP_LOGW(TAG, "MQTT_EVENT_DATA - DATA NOT CHUNKED");

                mqtt_client_event_data *data = new mqtt_client_event_data{
                    .topic = std::string(event->topic, event->topic_len),
                    .data = event->data,
                    .data_len = event->data_len,
                };
                thisClient->onDataReceived(thisClient, data);
                delete data;
            }
        }
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGW(TAG, "MQTT_EVENT_ERROR");
        // TODO: Handle the current state depending on the error:
        thisClient->currentState = MQTT_CONNECTION_LOST;

        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
            ESP_LOGW(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));

        }
        break;
    default:
        ESP_LOGW(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

std::string MQTTClient::read_cert_file(const char* filepath){
    File file = LittleFS.open(filepath, "r");
    if(!file){
        ESP_LOGE(TAG, "Failed to open file %s for reading", filepath);
        return NULL;
    }

    // size_t filesize = file.size();
    String data = file.readString();
    // log_d("File %s content: \n %s", filepath, data.c_str());
    file.close();
    return std::string(data.c_str(), data.length());

}

void MQTTClient::log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0) {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}

#elif defined(ESP8266)
void MQTTClient::setup(void){
    // SSL/TLS Certificates
    // Load certificate file:
    // But you must convert it to .der
    // openssl x509 -in ./certs/IoLed_controller/client.crt -out ./certs/IoLed_controller/cert.der -outform DER
    if (enable_certificates) {
        // Cert File
        File cert = LittleFS.open(client_cert_file_path.c_str(), "r");
        if (!cert) Serial.println("Couldn't load client cert file");
        else {
            size_t certSize = cert.size();
            client_cert_pem = (char *)malloc(certSize);
            if (certSize != cert.readBytes(client_cert_pem, certSize)) {
                Serial.println("Client cert load failed. Size mismatch");
            } else {
                Serial.println("Client cert loaded");
                clientCert = new BearSSL::X509List(client_cert_pem);
            }
            free(client_cert_pem);
            cert.close();
        }

        // Key File
        File key = LittleFS.open(client_key_file_path.c_str(), "r");
        if(!key) Serial.println("Couldn't load client key file");
        else {
            size_t keySize = key.size();
            client_key_pem = (char *)malloc(keySize);
            if (keySize != key.readBytes(client_key_pem, keySize)) {
                Serial.println("Client key load failed. Size mismatch");
            } else {
                Serial.println("Client key loaded");
                clientKey = new BearSSL::PrivateKey(client_key_pem);
            }
            free(client_key_pem);
            key.close();
        }

        wifiClientSecure.setClientRSACert(clientCert, clientKey);

        // CA File
        File ca = LittleFS.open(ca_file_path.c_str(), "r");
        if(!ca) Serial.println("Couldn't load CA cert file");
        else {
            size_t certSize = ca.size();
            ca_cert_pem = (char *)malloc(certSize);
            if (certSize != ca.readBytes(ca_cert_pem, certSize)) {
                Serial.println("CA cert failed. Size mismatch");
            } else {
                Serial.println("CA cert loaded");
                rootCert = new BearSSL::X509List(ca_cert_pem);
                wifiClientSecure.setTrustAnchors(rootCert);
            }
            free(ca_cert_pem);
            ca.close();
        }
    }

    // WebSockets
    if (enable_certificates){
        if(enable_websockets){
            wsClient = new WebSocketClient(wifiClientSecure, server.c_str(), port);
            wsStreamClient = new WebSocketStreamClient(*wsClient, websockets_path.c_str());
            mqttClient.setClient(*wsStreamClient);
            Serial.printf("Configuring MQTT using certificates and websockets path: %s\n", websockets_path.c_str());
        } else {
            mqttClient.setClient(wifiClientSecure);
            Serial.println("Configuring MQTT using certificates");
            mqttClient.setServer(server.c_str(), port);
        }
    } else {
        wifiClient = new WiFiClient();
        if(enable_websockets){
            wsClient = new WebSocketClient(*wifiClient, server.c_str(), port);
            wsStreamClient = new WebSocketStreamClient(*wsClient, websockets_path.c_str());
            mqttClient.setClient(*wsStreamClient);
            Serial.println("Configuring MQTT using websockets");
        } else {
            mqttClient.setClient(*wifiClient);
            Serial.println("Configuring MQTT using websockets without certificates");
            mqttClient.setServer(server.c_str(), port);
        }
    }

    // Configure MQTT event Callback
    mqttClient.setCallback([this](char* topic, byte* payload, unsigned int length) { this->callbackMQTT(topic, payload, length); });


}

void MQTTClient::callbackMQTT(char* topic, byte* payload, unsigned int length) {
    // Serial.print("Message arrived [");
    // Serial.print(topic);
    // Serial.print("] ");

    // char buff[length + 1];
    // for (unsigned int i = 0; i < length; i++) {
    //   //Serial.print((char)payload[i]);
    //   buff[i] = (char)payload[i];
    // }
    // buff[length] = '\0';

    // String message(buff);
    // Serial.print(message);
    // Serial.println();
  
    // Serial.printf("[%lu] +++ MQTT received %s %.*s\n", millis(), topic, length, payload);

    /*
    if (strcmp(topic, "/lamp") == 0) {
      //Lamp color request:
      if (message.equals("red")){
        Serial.println("Turning lamp to red");
        //colorWipe(strip.Color(255, 0, 0), 10);
      }
      else if (strcmp(buff, "blue") == 0){
          Serial.println("Turning lamp to blue");
          //colorWipe(strip.Color(0, 0, 255), 10);
      } else if (message.equals("green")){
          Serial.println("Turning lamp to green");
          //colorWipe(strip.Color(0, 255, 0), 10);
      }
      //client.publish((char*)"/lamp",(char*)"color changed");
    }
    */

    // Serial.print("Heap: "); Serial.println(ESP.getFreeHeap());

    char* message = new char[length + 1];
    memcpy(message, payload, length);
    message[length] = '\0';

    mqtt_client_event_data *data = new mqtt_client_event_data{
        .topic = std::string(topic),
        .data = (const char*)message,
        .data_len = length,
    };

    this->onDataReceived(this, data);
    delete[] message;
    delete data;

}


void MQTTClient::disconnect() {
    // Close old possible conections
    if (mqttClient.connected() ) mqttClient.disconnect();

    // Delete client pointers
    if (wsStreamClient){
        wsStreamClient->flush();
        wsStreamClient->stop();
        delete wsStreamClient;
        wsStreamClient = nullptr;
    }
    if (wsClient){
        // wsClient->flush();   // Not implemented
        wsClient->stop();
        delete wsClient;
        wsClient = nullptr;
    }
    if (wifiClient){
        wifiClient->flush();
        wifiClient->stop();
        delete wifiClient;
        wifiClient = nullptr;
    }

    //Renew the connection
    MQTTClient::setup();
}

void MQTTClient::reconnect() {
    // Loop until we're reconnected
    if (currentLoopMillis - previousMqttReconnectionMillis > mqttReconnectionTime){
        if (!mqttClient.connected() && ( mqttMaxRetries <= 0 || (mqttRetries <= mqttMaxRetries)) ) {
            MQTTClient::disconnect();
            
            bool mqttConnected = false;
            Serial.print("Attempting MQTT connection... ");
            
            std::string mqttWillTopic = base_topic_pub + "connected";
            uint8_t mqttWillQoS = 2;
            boolean mqttWillRetain = true;
            std::string mqttWillMessage = "false";

            if (enable_user_and_pass)
                mqttConnected = mqttClient.connect(id_name.c_str(),
                                                    user_name.c_str(),
                                                    user_password.c_str(),
                                                    mqttWillTopic.c_str(),
                                                    mqttWillQoS,
                                                    mqttWillRetain,
                                                    mqttWillMessage.c_str());
            else
                mqttConnected = mqttClient.connect(id_name.c_str(),
                                                    mqttWillTopic.c_str(),
                                                    mqttWillQoS,
                                                    mqttWillRetain,
                                                    mqttWillMessage.c_str());

            if (mqttConnected) {
                this->currentState = MQTT_CONNECTED;
                Serial.println("connected");
                // Once connected, publish an announcement...
                // TODO: publish a will message
                std::string topic_connected_pub = base_topic_pub + "connected";
                std::string msg_connected ="true";
                mqttClient.publish(topic_connected_pub.c_str(), msg_connected.c_str(), true);
                // ... and resubscribe
                std::string base_topic_sub = base_topic_pub + "#";
                // mqttClient.subscribe(base_topic_sub.c_str());
                this->addTopicSub(base_topic_sub.c_str());

                for (mqtt_client_topic_data& t : this->subTopics) {
                    t.subs_msg_id = this->topicId;
                    this->topicId++;
                    if(mqttClient.subscribe(t.topic.c_str(), t.qos)) {
                        t.subs_status = SUBSCRIBED;
                        this->onSubscribed(this, &t);
                    } else t.subs_status = ERROR;
                    Serial.printf("Topic[%d]: %s status to %d\n", t.subs_msg_id, t.topic.c_str(), t.subs_status);
                }

                mqttRetries = 0;
                Serial.printf("Time to connect MQTT client: %.2fs\n",(float)(millis() - connectionTime)/1000);

                this->onConnected(this);
            } else {
                this->currentState = MQTT_CONNECT_FAILED;
                Serial.printf("failed, rc=%d try again in %ds: %d/%s\n", 
                                mqttClient.state(), mqttReconnectionTime/1000, mqttRetries, 
                                mqttMaxRetries <= 0 ? "-" : String(mqttMaxRetries));
            }
            previousMqttReconnectionMillis = millis();
            mqttRetries++;
        }
    }
}

void MQTTClient::loop(void){
  currentLoopMillis = millis();
  if ( this->enabled) {
    if ( !this->mqttClient.connected() &&
          reconnect_mqtt && 
          WiFi.status() == WL_CONNECTED ) {
            connectionTime = currentLoopMillis;
            MQTTClient::reconnect();
      }

    if (mqttClient.connected()) mqttClient.loop();
  }

}
#endif

MQTTClientState MQTTClient::state(){
    //TODO: create state machine with real client status
    // using esp_mqtt_error_codes_t arriving on the 
    // within MQTT_EVENT_ERROR type event MQTTClient::eventHandler
    return currentState;
}

void MQTTClient::setMQTTClientId(std::string client_id) {
    id_name = client_id;
    StaticJsonDocument<192> docSave;
    docSave["id_name"] = this->id_name;
    IWebConfig::saveWebConfig(docSave.as<JsonObject>());
}

void MQTTClient::addTopicSub(const char* topic, int qos) {
    mqtt_client_topic_data newTopic = {
        .topic = topic,
        .qos = qos,
        .subs_msg_id = -1,
        .subs_status = ANY
    };

    if (currentState == MQTT_CONNECTED) {
        #ifdef ESP32
            newTopic.subs_msg_id = esp_mqtt_client_subscribe(client, newTopic.topic.c_str(), newTopic.qos);
        #elif defined(ESP8266)
            newTopic.subs_msg_id = this->topicId;
            this->topicId++;
            if(mqttClient.subscribe(newTopic.topic.c_str(), newTopic.qos)) 
                newTopic.subs_status = SUBSCRIBED;
            else 
                newTopic.subs_status = ERROR;
        #endif
    };

    subTopics.push_back(newTopic);
}

void MQTTClient::addTopicSub(const char* topic){
    addTopicSub(topic, 0);
}

mqtt_client_topic_data MQTTClient::getTopicSub(std::string topicName) {
    for (mqtt_client_topic_data t : subTopics) {
        if (t.topic == topicName) return t;
    }
}

bool MQTTClient::getTopicIsSubscribed(std::string topicName) {
    for (mqtt_client_topic_data t : subTopics) {
        if (t.topic == topicName) {
            // ESP_LOGW(TAG, "FOUND topic %s with status=%d", t.topic.c_str(), t.subs_status);
            if (t.subs_status == SUBSCRIBED) return true;
            else return false;
        }
    }
}

int MQTTClient::publish(const char *topic, const char *data, int len, int qos, int retain) {
    // ESP32 mqtt client return codes are followed for the return of this function
    #ifdef ESP32
        // mqtt client return -1 if the message was not published successfully,
        // or the message id number if the message was published successfully.
        // Also for QoS 0 messages, the return will be 0.
        ESP_LOGI(TAG, "MQTT TX -> topic: %s, message: %s", topic, data);
        return esp_mqtt_client_publish(client, topic, data, len, qos, retain);
	#elif defined(ESP8266)
        // PubSubClient publish return true if the message was published successfully, 
        // or false if there was an error.

        //WARNING: qos not used with PubSubClient!
        return mqttClient.publish(topic, data, retain) ? 0 : -1;
    #endif
}

int MQTTClient::publish(const char *topic, const char *data, int len) {
    return publish(topic, data, len, 0, 0);
}

int MQTTClient::publish(const char *topic, const char *data) {
    return publish(topic, data, 0, 0, 0);
}

void MQTTClient::parseWebConfig(JsonObjectConst configObject) {

    // JsonObject received:
    // serializeJsonPretty(configObject, Serial);

    // MQTTClient IWebConfig object:
    this->enabled = configObject["enabled"] | false;
    this->server = configObject["server"] | "server_address";
    this->port = configObject["port"] | 8888;
    this->id_name = configObject["id_name"] | "iotdevice";
    this->reconnect_mqtt = configObject["reconnect_mqtt"] | false;
    this->mqttMaxRetries = configObject["reconnect_retries"] | 10;
    this->mqttReconnectionTime = configObject["reconnect_time_ms"] | 10000;
    this->enable_user_and_pass = configObject["enable_user_and_pass"] | false;
    this->user_name = configObject["user_name"] | "user_name";
    this->user_password = configObject["user_password"] | "user_password";
    this->enable_certificates = configObject["enable_certificates"] | false;
    this->ca_file_path = configObject["ca_file"] | "certs/ca.crt";
    this->client_cert_file_path = configObject["cert_file"] | "certs/client.crt";
    this->client_key_file_path = configObject["key_file"] | "certs/client.key";
    this->enable_websockets = configObject["enable_websockets"] | false;
    this->websockets_path = configObject["websockets_path"] | "/";
    this->task_stack_size = configObject["task_stack_size"] | (7*1024);

    if (configObject["pub_topic"].size() > 0)
        for (unsigned int i = 0; i < configObject["pub_topic"].size(); i++)
            this->pub_topic[i] = configObject["pub_topic"][i].as<std::string>();
    else
        this->pub_topic[0] = configObject["pub_topic"].as<std::string>();

    if (configObject["sub_topic"].size() > 0)
        for (unsigned int i = 0; i < configObject["sub_topic"].size(); i++){
            this->sub_topic[i] = configObject["sub_topic"][i].as<std::string>();
            this->addTopicSub(this->sub_topic[i].c_str());
        }
    else
        this->sub_topic[0] = configObject["sub_topic"].as<std::string>();


    uint32_t chipId = 0;
    #ifdef ESP32
    for(int i=0; i<17; i=i+8) {
        chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
        }
    // Serial.printf("ESP32 Chip model = %s Rev %d\n", ESP.getChipModel(), ESP.getChipRevision());
    // Serial.printf("This chip has %d cores\n", ESP.getChipCores());
    // Serial.print("Chip ID: "); Serial.println(chipId);
    #else degined(ESP8266)
        chipId = ESP.getChipId();
    #endif

    this->base_topic_pub = "/" + id_name + "/" + std::to_string(chipId) + "/";
    // ESP_LOGE(TAG, "parseWebConfig MQTTClient enabled: %s every %dms and key: %s ", this->enabled? "true" : "false", this->metricsInterval, this->authKey.c_str());

}