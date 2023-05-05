MQTTClient: Changelog
=====================

HEAD
----

* first commit
* Add platformio library.json
* Add first MQTTClient class and include ArduinoJson lib dep
* Add release github action and build scripts
* Support for esp8266 target using PubSubClient, WebSocketStreamClient and ArduinoHttpClient for WebSockets client
* Fix PubSubclient callback and include onDataReceived
* Integrate addTopicSub method for esp8266. Handle connection state and topic subscription on connection
* Add client buffer set/get for esp8266 target
* Add publish method with topic, data and data length
* Created topic data struct and include it as a pointer in onSubscribed callback
* Replace data as pointer for onDataReceived callback
* Include data and topic structs as pointers in esp32 callbacks
* Fix pub_topic typo in MQTTClient::parseWebConfig method
* Add Readme basic info
* Create basic_class_with_mqtt example
* Update platformio library,json with example and espressif8266 platform
* Add executable permissions to github Release action scripts
* Fix compare_versions.sh script to compare an initial empty library version
