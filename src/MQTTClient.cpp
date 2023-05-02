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

        .client_cert_pem = (const char *)client_cert_pem,
        .client_key_pem = (const char *)client_key_pem,
        .use_global_ca_store = true,
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
                break;
            }
        }
        thisClient->onSubscribed(thisClient);
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
                    
                    mqtt_client_event_data data = {
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

                } else {
                    ESP_LOGW(TAG, "MQTT_EVENT_DATA - DATA CHUNKED NEXT");
                    memcpy(thisClient->bufOnData + thisClient->bufOnDataReceivedSize, event->data, event->data_len);
                    thisClient->bufOnDataReceivedSize += event->data_len;
                }
            } else {
                ESP_LOGW(TAG, "MQTT_EVENT_DATA - DATA NOT CHUNKED");

                mqtt_client_event_data data = {
                    .topic = std::string(event->topic, event->topic_len),
                    .data = event->data,
                    .data_len = event->data_len,
                };
                thisClient->onDataReceived(thisClient, data);
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
        this->sub_topic[i] = configObject["pub_topic"][i].as<std::string>();
    else
        this->sub_topic[0] = configObject["sub_topic"].as<std::string>();

    if (configObject["sub_topic"].size() > 0)
        for (unsigned int i = 0; i < configObject["sub_topic"].size(); i++)
        this->sub_topic[i] = configObject["sub_topic"][i].as<std::string>();
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

};
