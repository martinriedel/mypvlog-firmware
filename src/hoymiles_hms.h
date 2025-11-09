/**
 * Hoymiles HMS/HMT Protocol - CMT2300A Communication
 */

#ifndef HOYMILES_HMS_H
#define HOYMILES_HMS_H

#include <Arduino.h>

#ifdef RADIO_CMT2300A

class HoymilesHMS {
public:
    HoymilesHMS();
    void begin();
    void loop();
    bool addInverter(uint64_t serialNumber);
    void removeInverter(uint64_t serialNumber);
    uint8_t getInverterCount();

private:
    unsigned long m_lastPoll;
    uint16_t m_pollInterval;
    void pollInverters();
    void sendRequest(uint64_t serialNumber);
    bool receiveResponse(uint64_t serialNumber);
};

#endif // RADIO_CMT2300A

#endif // HOYMILES_HMS_H
