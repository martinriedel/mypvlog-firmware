/**
 * Hoymiles HM Protocol - NRF24L01+ Communication
 */

#ifndef HOYMILES_HM_H
#define HOYMILES_HM_H

#include <Arduino.h>
#include <functional>

#ifdef RADIO_NRF24

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

    // Callback
    std::function<void(uint64_t serial, float power, float voltage, float current)> m_dataCallback;

    // Protocol methods
    void pollInverters();
    void sendRequest(uint64_t serialNumber);
    bool receiveResponse(uint64_t serialNumber);
};

#endif // RADIO_NRF24

#endif // HOYMILES_HM_H
