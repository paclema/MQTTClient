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
