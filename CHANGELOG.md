MQTTClient: Changelog
=====================

HEAD
----

* Remove unnecessary class instance
* Remove IWebConfig dependency and replace parseWebConfig with setConfig method

v0.0.4 (2023-05-08)
------

* Enhance MQTT connection type messages for esp8266
* Use certificates configuration only if enable_certificates config is enabled fotr esp32
* Use user and password configurations only if enable_user_and_pass config is enabled fotr esp32
* Clean up preprocessor and certs achar arrays creation
* Removed WebConfigServer dependency and catch IWebConfig use
* Fix IWebConfig inheritance when WebConfigServer is not used
* Merge branch 'fix_circular_dep'
* Replace precompiled warning with mmessages

v0.0.3 (2023-05-05)
------

* Remove temporary username/password with a more meaningful example

v0.0.2 (2023-05-05)
------

* Fix PlatformIO badge release number. 
* Change README.md to use LF

v0.0.1 (2023-05-05)
------

* Include back platformio example in library.json

v0.0.0 (2023-05-05)
------

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
* Fix platformio library.json with correct example name and version
* Remove example for platformio and data folder for release package
