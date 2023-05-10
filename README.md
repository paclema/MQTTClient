# MQTTClient
[![Release Build](https://github.com/paclema/MQTTClient/actions/workflows/release.yml/badge.svg)](https://github.com/paclema/MQTTClient/actions/workflows/release.yml)
[![PlatformIO Registry](https://badges.registry.platformio.org/packages/paclema/library/MQTTClient.svg?version=0.1.0)](https://registry.platformio.org/libraries/paclema/MQTTClient)
[![Donate](https://img.shields.io/badge/Donate-PayPal-blue.svg?color=yellow)](https://www.paypal.com/donate/?business=8PXZ598XDGAS2&no_recurring=0&currency_code=EUR&source=url)


MQTT Client library that wrappers ESP-IDF mqtt client component into a C++ class for ESP32 and PubSubClient for ESP8266.

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
This library requires [WebConfigServer](https://github.com/paclema/WebConfigServer) library for configuration. The next configurations can be added into the configuration file `config/config.json`  stored in the littlefs partition:

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
