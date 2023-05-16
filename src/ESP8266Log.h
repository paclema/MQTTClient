#if !defined (ESP8266LOG_H) && defined(ESP8266)
#define ESP8266LOG_H

#ifdef ESP8266
  #define ESP_LOGI(TAG, ...) do { \
    Serial.printf("[%6lu][I][%s]: ", (unsigned long) (micros() / 1000ULL), TAG); \
    Serial.printf(__VA_ARGS__); \
    Serial.println(); \
  } while (0)
#define ESP_LOGW(TAG, ...) do { \
    Serial.printf("[%6lu][W][%s]: ", (unsigned long) (micros() / 1000ULL), TAG); \
    Serial.printf(__VA_ARGS__); \
    Serial.println(); \
  } while (0)
#endif

#endif