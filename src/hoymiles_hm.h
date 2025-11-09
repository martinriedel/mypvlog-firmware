/**
 * Hoymiles HM Protocol - NRF24L01+ Communication
 */

#ifndef HOYMILES_HM_H
#define HOYMILES_HM_H

#include <Arduino.h>

class HoymilesHM {
public:
    HoymilesHM();
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

#endif // HOYMILES_HM_H
