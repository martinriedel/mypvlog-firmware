/**
 * Hoymiles HM Protocol - NRF24L01+ Communication
 *
 * Stub implementation - To be completed with actual protocol
 */

#include "hoymiles_hm.h"
#include "config.h"

#ifdef RADIO_NRF24

HoymilesHM::HoymilesHM()
    : m_lastPoll(0)
    , m_pollInterval(HOYMILES_POLL_INTERVAL)
    , m_inverterCount(0)
{
}

void HoymilesHM::begin() {
    DEBUG_PRINTLN("Hoymiles HM: Initializing (stub implementation)");

    // TODO: Initialize NRF24L01+ radio
    // TODO: Configure SPI
    // TODO: Set up radio parameters

    DEBUG_PRINTLN("Hoymiles HM: Initialized (stub)");
}

void HoymilesHM::loop() {
    unsigned long now = millis();

    if (now - m_lastPoll > m_pollInterval) {
        m_lastPoll = now;
        pollInverters();
    }
}

bool HoymilesHM::addInverter(uint64_t serialNumber) {
    if (m_inverterCount >= HOYMILES_MAX_INVERTERS) {
        DEBUG_PRINTLN("Hoymiles HM: Maximum inverters reached");
        return false;
    }

    // TODO: Add inverter to list
    // TODO: Store serial number

    m_inverterCount++;

    DEBUG_PRINT("Hoymiles HM: Added inverter ");
    DEBUG_PRINTLN((unsigned long)serialNumber);

    return true;
}

void HoymilesHM::removeInverter(uint64_t serialNumber) {
    // TODO: Remove inverter from list

    if (m_inverterCount > 0) {
        m_inverterCount--;
    }

    DEBUG_PRINT("Hoymiles HM: Removed inverter ");
    DEBUG_PRINTLN((unsigned long)serialNumber);
}

uint8_t HoymilesHM::getInverterCount() {
    return m_inverterCount;
}

void HoymilesHM::setPollInterval(uint16_t interval) {
    m_pollInterval = interval;
    DEBUG_PRINT("Hoymiles HM: Poll interval set to ");
    DEBUG_PRINT(m_pollInterval);
    DEBUG_PRINTLN("ms");
}

void HoymilesHM::pollInverters() {
    // TODO: Implement actual polling logic
    // This is where we'll:
    // 1. Iterate through registered inverters
    // 2. Send request via NRF24
    // 3. Wait for response
    // 4. Parse data
    // 5. Call callback with data

    DEBUG_PRINTLN("Hoymiles HM: Polling inverters (stub)");
}

void HoymilesHM::sendRequest(uint64_t serialNumber) {
    // TODO: Implement request sending
    // Format Hoymiles protocol packet
    // Send via NRF24

    DEBUG_PRINT("Hoymiles HM: Sending request to ");
    DEBUG_PRINTLN((unsigned long)serialNumber);
}

bool HoymilesHM::receiveResponse(uint64_t serialNumber) {
    // TODO: Implement response reception
    // Wait for NRF24 interrupt
    // Read packet
    // Validate CRC
    // Return success/failure

    DEBUG_PRINT("Hoymiles HM: Receiving response from ");
    DEBUG_PRINTLN((unsigned long)serialNumber);

    return false; // Stub: always return false
}

void HoymilesHM::setDataCallback(std::function<void(uint64_t serial, float power, float voltage, float current)> callback) {
    m_dataCallback = callback;
}

#endif // RADIO_NRF24
