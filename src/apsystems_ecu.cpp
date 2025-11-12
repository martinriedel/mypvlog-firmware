/**
 * APSystems ECU-R Client Implementation
 */

#include "apsystems_ecu.h"

#ifdef APSYSTEMS_ECU

#include "config.h"

APSystemsECU::APSystemsECU()
    : m_initialized(false)
    , m_lastPoll(0)
    , m_pollInterval(APSYSTEMS_POLL_INTERVAL)
    , m_inverterCount(0)
    , m_totalPower(0.0)
    , m_lifetimeEnergy(0.0)
    , m_todayEnergy(0.0)
{
    memset(m_ecuIP, 0, sizeof(m_ecuIP));
    memset(m_ecuID, 0, sizeof(m_ecuID));
    memset(m_inverters, 0, sizeof(m_inverters));
}

APSystemsECU::~APSystemsECU() {
}

bool APSystemsECU::begin(const char* ecuIP) {
    if (!ecuIP) {
        DEBUG_PRINTLN("[APSystems] Error: ECU IP is null");
        return false;
    }

    strncpy(m_ecuIP, ecuIP, sizeof(m_ecuIP) - 1);
    m_ecuIP[sizeof(m_ecuIP) - 1] = '\0';

    DEBUG_PRINT("[APSystems] Initializing ECU client for IP: ");
    DEBUG_PRINTLN(m_ecuIP);

    // Query ECU to get ID and verify connection
    if (!queryECU()) {
        DEBUG_PRINTLN("[APSystems] Failed to query ECU");
        return false;
    }

    m_initialized = true;
    DEBUG_PRINTLN("[APSystems] Initialization successful");
    return true;
}

void APSystemsECU::loop() {
    if (!m_initialized) {
        return;
    }

    unsigned long now = millis();
    if (now - m_lastPoll >= m_pollInterval) {
        m_lastPoll = now;

        // Query inverter data
        if (queryInverters()) {
            // Query signal strength
            querySignalStrength();

            // Invoke callbacks for each inverter
            if (m_dataCallback) {
                for (uint8_t i = 0; i < m_inverterCount; i++) {
                    m_dataCallback(m_inverters[i].uid, m_inverters[i]);
                }
            }
        }
    }
}

void APSystemsECU::setPollInterval(uint16_t interval) {
    m_pollInterval = interval;
}

void APSystemsECU::setECUID(const char* ecuID) {
    if (ecuID) {
        strncpy(m_ecuID, ecuID, sizeof(m_ecuID) - 1);
        m_ecuID[sizeof(m_ecuID) - 1] = '\0';
    }
}

void APSystemsECU::setDataCallback(std::function<void(const char* uid, const APSystemsInverterData& data)> callback) {
    m_dataCallback = callback;
}

bool APSystemsECU::isConnected() {
    return m_initialized;
}

uint8_t APSystemsECU::getInverterCount() {
    return m_inverterCount;
}

const char* APSystemsECU::getECUID() {
    return m_ecuID;
}

float APSystemsECU::getTotalPower() {
    return m_totalPower;
}

float APSystemsECU::getLifetimeEnergy() {
    return m_lifetimeEnergy;
}

float APSystemsECU::getTodayEnergy() {
    return m_todayEnergy;
}

// Protocol Implementation

bool APSystemsECU::sendCommand(const char* command, uint8_t* response, size_t& responseLen) {
    WiFiClient client;

    if (!client.connect(m_ecuIP, APSYSTEMS_ECU_PORT)) {
        DEBUG_PRINTLN("[APSystems] Failed to connect to ECU");
        return false;
    }

    client.setTimeout(APSYSTEMS_SOCKET_TIMEOUT);

    // Send command
    client.print(command);
    client.flush();

    // Wait for response
    delay(100);

    // Read response
    responseLen = 0;
    unsigned long startTime = millis();
    while (client.connected() && (millis() - startTime < APSYSTEMS_SOCKET_TIMEOUT)) {
        if (client.available()) {
            uint8_t byte = client.read();
            if (responseLen < 1024) {  // Max buffer size
                response[responseLen++] = byte;
            }
        }
        yield();
    }

    client.stop();

    return responseLen > 0;
}

bool APSystemsECU::queryECU() {
    DEBUG_PRINTLN("[APSystems] Querying ECU...");

    uint8_t response[512];
    size_t responseLen = 0;

    const char* command = "APS1100160001END\n";

    if (!sendCommand(command, response, responseLen)) {
        return false;
    }

    return parseECUResponse(response, responseLen);
}

bool APSystemsECU::queryInverters() {
    DEBUG_PRINTLN("[APSystems] Querying inverters...");

    uint8_t response[1024];
    size_t responseLen = 0;

    char command[64];
    snprintf(command, sizeof(command), "APS1100280002%sEND\n", m_ecuID);

    if (!sendCommand(command, response, responseLen)) {
        return false;
    }

    return parseInverterResponse(response, responseLen);
}

bool APSystemsECU::querySignalStrength() {
    DEBUG_PRINTLN("[APSystems] Querying signal strength...");

    uint8_t response[512];
    size_t responseLen = 0;

    char command[64];
    snprintf(command, sizeof(command), "APS1100280030%sEND\n", m_ecuID);

    if (!sendCommand(command, response, responseLen)) {
        return false;
    }

    return parseSignalResponse(response, responseLen);
}

// Parsing Methods

bool APSystemsECU::parseECUResponse(const uint8_t* data, size_t len) {
    if (len < 20) {
        DEBUG_PRINTLN("[APSystems] ECU response too short");
        return false;
    }

    // Check header "APS"
    if (data[0] != 'A' || data[1] != 'P' || data[2] != 'S') {
        DEBUG_PRINTLN("[APSystems] Invalid ECU response header");
        return false;
    }

    // Extract ECU ID (starts at offset 13)
    if (len >= 25) {
        memset(m_ecuID, 0, sizeof(m_ecuID));
        for (int i = 0; i < 12 && (13 + i) < len; i++) {
            m_ecuID[i] = data[13 + i];
        }
        DEBUG_PRINT("[APSystems] ECU ID: ");
        DEBUG_PRINTLN(m_ecuID);
    }

    // Extract lifetime energy (offset varies by ECU model)
    if (len >= 30) {
        uint32_t lifetimeRaw = apsIntFromBytes(data, 27, 4);
        m_lifetimeEnergy = lifetimeRaw / 10.0;  // Convert to kWh
        DEBUG_PRINT("[APSystems] Lifetime energy: ");
        DEBUG_PRINT(m_lifetimeEnergy);
        DEBUG_PRINTLN(" kWh");
    }

    return true;
}

bool APSystemsECU::parseInverterResponse(const uint8_t* data, size_t len) {
    if (len < 20) {
        DEBUG_PRINTLN("[APSystems] Inverter response too short");
        return false;
    }

    // Check header
    if (data[0] != 'A' || data[1] != 'P' || data[2] != 'S') {
        DEBUG_PRINTLN("[APSystems] Invalid inverter response header");
        return false;
    }

    // Extract inverter count
    if (len >= 18) {
        m_inverterCount = apsIntFromBytes(data, 17, 1);
        if (m_inverterCount > APSYSTEMS_MAX_INVERTERS) {
            DEBUG_PRINT("[APSystems] Warning: inverter count ");
            DEBUG_PRINT(m_inverterCount);
            DEBUG_PRINTLN(" exceeds maximum, truncating");
            m_inverterCount = APSYSTEMS_MAX_INVERTERS;
        }

        DEBUG_PRINT("[APSystems] Inverter count: ");
        DEBUG_PRINTLN(m_inverterCount);
    }

    // Parse each inverter
    uint16_t offset = 18;
    m_totalPower = 0.0;

    for (uint8_t i = 0; i < m_inverterCount && offset < len; i++) {
        APSystemsInverterData& inv = m_inverters[i];

        // UID (12 bytes)
        if (offset + 12 <= len) {
            memset(inv.uid, 0, sizeof(inv.uid));
            memcpy(inv.uid, &data[offset], 12);
            offset += 12;
        }

        // Online status (1 byte)
        if (offset + 1 <= len) {
            inv.online = (data[offset] == 1);
            offset += 1;
        }

        // Determine channel count based on inverter model
        // YC600 and QS1 have 2 channels, YC1000/QT2 have 4 channels
        inv.channelCount = 2;  // Default
        if (strncmp(inv.uid, "YC1000", 6) == 0 || strncmp(inv.uid, "QT2", 3) == 0) {
            inv.channelCount = 4;
        }

        // Power (2 bytes per channel, in 0.1W units)
        for (uint8_t ch = 0; ch < inv.channelCount && offset + 2 <= len; ch++) {
            uint16_t powerRaw = apsIntFromBytes(data, offset, 2);
            inv.power[ch] = powerRaw / 10.0;
            m_totalPower += inv.power[ch];
            offset += 2;
        }

        // Voltage (2 bytes per channel, in 0.1V units)
        for (uint8_t ch = 0; ch < inv.channelCount && offset + 2 <= len; ch++) {
            uint16_t voltageRaw = apsIntFromBytes(data, offset, 2);
            inv.voltage[ch] = voltageRaw / 10.0;
            offset += 2;
        }

        // Frequency (2 bytes, in 0.01Hz units)
        if (offset + 2 <= len) {
            uint16_t freqRaw = apsIntFromBytes(data, offset, 2);
            inv.frequency = freqRaw / 100.0;
            offset += 2;
        }

        // Temperature (2 bytes, in 0.1°C units)
        if (offset + 2 <= len) {
            int16_t tempRaw = (int16_t)apsIntFromBytes(data, offset, 2);
            inv.temperature = tempRaw / 10.0;
            offset += 2;
        }

        DEBUG_PRINT("[APSystems] Inverter ");
        DEBUG_PRINT(inv.uid);
        DEBUG_PRINT(": ");
        DEBUG_PRINT(inv.online ? "online" : "offline");
        DEBUG_PRINT(", Power: ");
        DEBUG_PRINT(inv.power[0] + inv.power[1]);
        DEBUG_PRINTLN("W");
    }

    return true;
}

bool APSystemsECU::parseSignalResponse(const uint8_t* data, size_t len) {
    if (len < 20) {
        DEBUG_PRINTLN("[APSystems] Signal response too short");
        return false;
    }

    // Check header
    if (data[0] != 'A' || data[1] != 'P' || data[2] != 'S') {
        DEBUG_PRINTLN("[APSystems] Invalid signal response header");
        return false;
    }

    // Extract signal strength for each inverter
    uint16_t offset = 17;
    for (uint8_t i = 0; i < m_inverterCount && offset < len; i++) {
        if (offset + 1 <= len) {
            m_inverters[i].signalStrength = data[offset];
            offset += 1;
        }
    }

    return true;
}

// Utility Methods

uint32_t APSystemsECU::apsIntFromBytes(const uint8_t* data, uint8_t offset, uint8_t len) {
    if (len > 4) len = 4;  // Maximum 4 bytes for uint32_t

    uint32_t result = 0;
    for (uint8_t i = 0; i < len; i++) {
        result = (result << 8) | data[offset + i];
    }

    return result;
}

bool APSystemsECU::validateChecksum(const uint8_t* data, size_t len) {
    // APSystems uses a checksum at bytes 5-9
    // This is a simplified validation - full implementation would verify the checksum
    if (len < 10) {
        return false;
    }

    // For now, just check if the data length matches what's in the checksum field
    uint32_t expectedLen = apsIntFromBytes(data, 5, 4);
    return (expectedLen + 9 <= len);
}

#endif // APSYSTEMS_ECU
