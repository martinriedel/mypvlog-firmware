/**
 * Hoymiles HMS/HMT Protocol - CMT2300A Communication
 *
 * Stub implementation - To be completed with actual protocol
 */

#include "hoymiles_hms.h"
#include "config.h"

#ifdef RADIO_CMT2300A

HoymilesHMS::HoymilesHMS()
    : m_lastPoll(0)
    , m_pollInterval(HOYMILES_POLL_INTERVAL)
    , m_inverterCount(0)
{
}

void HoymilesHMS::begin() {
    DEBUG_PRINTLN("Hoymiles HMS/HMT: Initializing (stub implementation)");

    // TODO: Initialize CMT2300A radio
    // TODO: Configure SPI
    // TODO: Set up radio parameters for 868MHz

    DEBUG_PRINTLN("Hoymiles HMS/HMT: Initialized (stub)");
}

void HoymilesHMS::loop() {
    unsigned long now = millis();

    if (now - m_lastPoll > m_pollInterval) {
        m_lastPoll = now;
        pollInverters();
    }
}

bool HoymilesHMS::addInverter(uint64_t serialNumber) {
    if (m_inverterCount >= HOYMILES_MAX_INVERTERS) {
        DEBUG_PRINTLN("Hoymiles HMS/HMT: Maximum inverters reached");
        return false;
    }

    // TODO: Add inverter to list
    // TODO: Store serial number

    m_inverterCount++;

    DEBUG_PRINT("Hoymiles HMS/HMT: Added inverter ");
    DEBUG_PRINTLN((unsigned long)serialNumber);

    return true;
}

void HoymilesHMS::removeInverter(uint64_t serialNumber) {
    // TODO: Remove inverter from list

    if (m_inverterCount > 0) {
        m_inverterCount--;
    }

    DEBUG_PRINT("Hoymiles HMS/HMT: Removed inverter ");
    DEBUG_PRINTLN((unsigned long)serialNumber);
}

uint8_t HoymilesHMS::getInverterCount() {
    return m_inverterCount;
}

void HoymilesHMS::setPollInterval(uint16_t interval) {
    m_pollInterval = interval;
    DEBUG_PRINT("Hoymiles HMS/HMT: Poll interval set to ");
    DEBUG_PRINT(m_pollInterval);
    DEBUG_PRINTLN("ms");
}

void HoymilesHMS::pollInverters() {
    // TODO: Implement actual polling logic for HMS/HMT
    // This uses CMT2300A instead of NRF24

    DEBUG_PRINTLN("Hoymiles HMS/HMT: Polling inverters (stub)");
}

void HoymilesHMS::sendRequest(uint64_t serialNumber) {
    // TODO: Implement request sending via CMT2300A

    DEBUG_PRINT("Hoymiles HMS/HMT: Sending request to ");
    DEBUG_PRINTLN((unsigned long)serialNumber);
}

bool HoymilesHMS::receiveResponse(uint64_t serialNumber) {
    // TODO: Implement response reception via CMT2300A

    DEBUG_PRINT("Hoymiles HMS/HMT: Receiving response from ");
    DEBUG_PRINTLN((unsigned long)serialNumber);

    return false; // Stub: always return false
}

void HoymilesHMS::setDataCallback(std::function<void(uint64_t serial, float power, float voltage, float current)> callback) {
    m_dataCallback = callback;
}

#endif // RADIO_CMT2300A
