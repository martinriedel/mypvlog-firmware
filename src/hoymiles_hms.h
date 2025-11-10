/**
 * Hoymiles HMS/HMT Protocol - CMT2300A Communication
 *
 * Supports HMS-800 to HMS-2000 and HMT series inverters
 * Uses CMT2300A radio module (868MHz)
 */

#ifndef HOYMILES_HMS_H
#define HOYMILES_HMS_H

#include <Arduino.h>
#include <functional>

#ifdef RADIO_CMT2300A

#include "hoymiles_protocol.h"

// Maximum number of inverters to manage
#ifndef HOYMILES_MAX_INVERTERS
#define HOYMILES_MAX_INVERTERS  8
#endif

// Default poll interval (milliseconds)
#ifndef HOYMILES_POLL_INTERVAL
#define HOYMILES_POLL_INTERVAL  5000
#endif

class HoymilesHMS {
public:
    HoymilesHMS();
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

    // Inverter storage
    uint64_t m_inverters[HOYMILES_MAX_INVERTERS];

    // Callback
    std::function<void(uint64_t serial, float power, float voltage, float current)> m_dataCallback;

    // Protocol methods
    void pollInverters();
    void sendRequest(uint64_t serialNumber);
    bool receiveResponse(uint64_t serialNumber);
};

#endif // RADIO_CMT2300A

#endif // HOYMILES_HMS_H
