/**
 * Hoymiles HM Protocol - NRF24L01+ Communication
 */

#ifndef HOYMILES_HM_H
#define HOYMILES_HM_H

#include <Arduino.h>
#include <functional>

#ifdef RADIO_NRF24

#include <RF24.h>
#include "hoymiles_protocol.h"

#define HOYMILES_MAX_INVERTERS  8

// Pin definitions (can be overridden in config.h)
#ifndef NRF24_CE_PIN
  #ifdef ESP32
    #define NRF24_CE_PIN   2
    #define NRF24_CS_PIN   5
  #elif defined(ESP8266)
    #define NRF24_CE_PIN   4  // D2
    #define NRF24_CS_PIN   5  // D1
  #endif
#endif

class HoymilesHM {
public:
    HoymilesHM();
    void begin();
    void loop();

    // Inverter management
    bool addInverter(uint64_t serialNumber);
    void removeInverter(uint64_t serialNumber);
    uint8_t getInverterCount();

    // Configuration
    void setPollInterval(uint16_t interval);

    // Callback for inverter data
    void setDataCallback(std::function<void(uint64_t serial, float power, float voltage, float current)> callback);

private:
    unsigned long m_lastPoll;
    uint16_t m_pollInterval;
    uint8_t m_inverterCount;

    // RF24 radio instance
    RF24* m_radio;

    // Inverter list
    uint64_t m_inverters[HOYMILES_MAX_INVERTERS];

    // Callback
    std::function<void(uint64_t serial, float power, float voltage, float current)> m_dataCallback;

    // Protocol methods
    void pollInverters();
    void sendRequest(uint64_t serialNumber);
    bool receiveResponse(uint64_t serialNumber);
};

#endif // RADIO_NRF24

#endif // HOYMILES_HM_H
