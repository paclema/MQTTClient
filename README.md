# MQTTClient
[![Release Build](https://github.com/paclema/MQTTClient/actions/workflows/release.yml/badge.svg)](https://github.com/paclema/MQTTClient/actions/workflows/release.yml)
[![PlatformIO Registry](https://badges.registry.platformio.org/packages/paclema/library/MQTTClient.svg?version=1.0.2)](https://registry.platformio.org/libraries/paclema/MQTTClient)
[![Donate](https://img.shields.io/badge/Donate-PayPal-blue.svg?color=yellow)](https://www.paypal.com/donate/?business=8PXZ598XDGAS2&no_recurring=0&currency_code=EUR&source=url)


MQTT client library that wrappers ESP-IDF mqtt client component into a C++ class for ESP32 and PubSubClient for ESP8266.

## Features:
* Username/Password authentication
* MQTT over TLS/SSL
* MQTT over WebSockets
* MQTT over Secure WebSockets
* Automatic reconnection
* Topic subscription management
* Configurable WebConfigServer class for configuration
* Configuration json and SSL certificates stored in littlefs partition


## Library configuration:

To configure the MQTTClient client, the following parameters can be added into an ArduinoJson document:

```json
{
  "mqtt": {
    "enabled": true,
    "reconnect_mqtt": true,
    "reconnect_retries": 10,
    "reconnect_time_ms": 10000,
    "server": "broker.ddns.net",
    "port": 1883,
    "id_name": "iot-button",
    "enable_user_and_pass": true,
    "user_name": "userName",
    "user_password": "userPassword",
    "enable_certificates": false,
    "ca_file": "/certs/ca.crt",
    "cert_file": "/certs/cert.der",
    "key_file": "/certs/private.der",
    "enable_websockets": false,
    "websockets_path": "/",
    "pub_topic": [
      "/iot-button/feed"
    ],
    "sub_topic": [
      "/iot-button/topi1",
      "/iot-button/topi2",
      "/iot-button/topi3"
    ],
    "task_stack_size": 7168
  }
}
```
## Examples:

Check the basic example [basic_class_with_mqtt](examples/basic_class_with_mqtt/) to see how to do it and also to use the MQTTClient within your own class.

On the other hand, It is recommended to use MQTTClient along with [WebConfigServer](https://github.com/paclema/WebConfigServer) library to easily configure the MQTTClient configurations adding them to the littlefs _data_ partition into the WebConfigServer configuration file `config/config.json`. WebConfigServer already has instanced an MQTTClient object and you can also use it to publish or subscribe to MQTT topics. Check the example [basic_class_with_mqtt_WebConfigServer](examples/basic_class_with_mqtt_WebConfigServer) for more details.
