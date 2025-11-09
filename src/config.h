/**
 * MyPVLog Firmware - Configuration
 */

#ifndef CONFIG_H
#define CONFIG_H

// Firmware version (set by build system)
#ifndef VERSION
#define VERSION "1.0.0-dev"
#endif

// WiFi AP Configuration
#define WIFI_AP_SSID_PREFIX "MyPVLog-"
#define WIFI_AP_PASSWORD "mypvlog123"
#define WIFI_AP_CHANNEL 1
#define WIFI_AP_MAX_CONNECTIONS 4

// Web Server Configuration
#define WEB_SERVER_PORT 80
#define WEB_SERVER_CAPTIVE_PORTAL true

// MQTT Configuration
#define MQTT_DEFAULT_PORT 1883
#define MQTT_DEFAULT_SSL_PORT 8883
#define MQTT_DEFAULT_KEEPALIVE 60
#define MQTT_RECONNECT_INTERVAL 5000
#define MQTT_MAX_RECONNECT_ATTEMPTS 10

// MyPVLog.net Configuration
#define MYPVLOG_API_URL "https://api.mypvlog.net"
#define MYPVLOG_MQTT_BROKER "mqtt.mypvlog.net"
#define MYPVLOG_MQTT_PORT 8883

// NRF24L01+ Pin Configuration (ESP32)
#ifdef ESP32
    #define NRF24_CE_PIN 2
    #define NRF24_CS_PIN 5
    #define NRF24_IRQ_PIN 16
    #define NRF24_MISO_PIN 19
    #define NRF24_MOSI_PIN 23
    #define NRF24_SCK_PIN 18
#endif

// NRF24L01+ Pin Configuration (ESP8266)
#ifdef ESP8266
    #define NRF24_CE_PIN D2
    #define NRF24_CS_PIN D8
    #define NRF24_IRQ_PIN D1
    // ESP8266 uses hardware SPI (MISO=D6, MOSI=D7, SCK=D5)
#endif

// CMT2300A Pin Configuration (ESP32 only)
#ifdef RADIO_CMT2300A
    #define CMT2300A_CS_PIN 15
    #define CMT2300A_FCSB_PIN 4
    // Shares SPI bus with NRF24
#endif

// Hoymiles Configuration
#define HOYMILES_POLL_INTERVAL 5000     // Generic mode: 5 seconds
#define HOYMILES_POLL_INTERVAL_FAST 2000 // MyPVLog Direct mode: 2 seconds
#define HOYMILES_MAX_INVERTERS 8
#define HOYMILES_RETRY_ATTEMPTS 3
#define HOYMILES_RESPONSE_TIMEOUT 1000

// LED Configuration
#ifdef ESP32
    #define LED_BUILTIN 2
#elif defined(ESP8266)
    #define LED_BUILTIN LED_BUILTIN
#endif

// Debug Configuration
#define DEBUG_SERIAL Serial
#define DEBUG_ENABLED true

#if DEBUG_ENABLED
    #define DEBUG_PRINT(x) DEBUG_SERIAL.print(x)
    #define DEBUG_PRINTLN(x) DEBUG_SERIAL.println(x)
    #define DEBUG_PRINTF(...) DEBUG_SERIAL.printf(__VA_ARGS__)
#else
    #define DEBUG_PRINT(x)
    #define DEBUG_PRINTLN(x)
    #define DEBUG_PRINTF(...)
#endif

#endif // CONFIG_H
