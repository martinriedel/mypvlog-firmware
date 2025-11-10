/**
 * Hoymiles HMS/HMT Protocol - CMT2300A Communication
 *
 * Full implementation for HMS-800 to HMS-2000 and HMT series inverters
 * Uses CMT2300A radio module (868MHz) via RadioLib
 *
 * Based on reverse-engineering work from OpenDTU project
 */

#include "hoymiles_hms.h"
#include "hoymiles_protocol.h"
#include "config.h"

#ifdef RADIO_CMT2300A

#include <RadioLib.h>

// Pin definitions for CMT2300A
#ifndef CMT2300A_CS_PIN
  #ifdef ESP32
    #define CMT2300A_CS_PIN    15   // SPI CS
    #define CMT2300A_GPIO1_PIN 21   // GPIO1 (used for interrupts)
    #define CMT2300A_GPIO2_PIN 22   // GPIO2
    #define CMT2300A_GPIO3_PIN 23   // GPIO3
  #elif defined(ESP8266)
    #define CMT2300A_CS_PIN    15   // D8
    #define CMT2300A_GPIO1_PIN 4    // D2
    #define CMT2300A_GPIO2_PIN 5    // D1
    #define CMT2300A_GPIO3_PIN 12   // D6
  #endif
#endif

// CMT2300A radio instance
static CMT2300A* g_radio = nullptr;

HoymilesHMS::HoymilesHMS()
    : m_lastPoll(0)
    , m_pollInterval(HOYMILES_POLL_INTERVAL)
    , m_inverterCount(0)
{
    // Initialize inverter array
    for (uint8_t i = 0; i < HOYMILES_MAX_INVERTERS; i++) {
        m_inverters[i] = 0;
    }
}

void HoymilesHMS::begin() {
    DEBUG_PRINTLN("Hoymiles HMS/HMT: Initializing CMT2300A radio...");

    // Create CMT2300A instance
    g_radio = new Module(CMT2300A_CS_PIN, CMT2300A_GPIO1_PIN, RADIOLIB_NC, CMT2300A_GPIO2_PIN);
    CMT2300A radio = new CMT2300A(g_radio);

    // Initialize CMT2300A with 868MHz configuration for Hoymiles HMS
    int state = radio.begin(
        868.0,           // Frequency: 868 MHz (European ISM band)
        38.4,            // Bit rate: 38.4 kbps
        10.0,            // Frequency deviation: 10 kHz
        135.0,           // RX bandwidth: 135 kHz
        10,              // Output power: 10 dBm
        32               // Preamble length: 32 bits
    );

    if (state != RADIOLIB_ERR_NONE) {
        DEBUG_PRINT("Hoymiles HMS/HMT: ERROR - CMT2300A initialization failed! Code: ");
        DEBUG_PRINTLN(state);
        return;
    }

    // Configure radio for Hoymiles protocol
    // HMS uses GFSK modulation
    radio.setDataShaping(RADIOLIB_SHAPING_0_5);

    // Set sync word (Hoymiles HMS specific)
    uint8_t syncWord[] = {0xD3, 0x91};
    radio.setSyncWord(syncWord, 2);

    // Enable CRC
    radio.setCRC(true);

    // Put radio in receive mode
    state = radio.startReceive();
    if (state != RADIOLIB_ERR_NONE) {
        DEBUG_PRINT("Hoymiles HMS/HMT: ERROR - Failed to start receive mode! Code: ");
        DEBUG_PRINTLN(state);
        return;
    }

    DEBUG_PRINTLN("Hoymiles HMS/HMT: CMT2300A initialized successfully");
    DEBUG_PRINTLN("Hoymiles HMS/HMT: Radio configured for 868 MHz");
    DEBUG_PRINTLN("Hoymiles HMS/HMT: Ready to communicate with HMS/HMT inverters");
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
        DEBUG_PRINTLN("Hoymiles HMS/HMT: ERROR - Maximum inverters reached");
        return false;
    }

    // Check if inverter already exists
    for (uint8_t i = 0; i < m_inverterCount; i++) {
        if (m_inverters[i] == serialNumber) {
            DEBUG_PRINTLN("Hoymiles HMS/HMT: Inverter already registered");
            return false;
        }
    }

    // Add to list
    m_inverters[m_inverterCount] = serialNumber;
    m_inverterCount++;

    DEBUG_PRINT("Hoymiles HMS/HMT: Added inverter #");
    DEBUG_PRINT(m_inverterCount);
    DEBUG_PRINT(" - Serial: ");
    DEBUG_PRINTLN((unsigned long)(serialNumber & 0xFFFFFFFF)); // Print lower 32 bits

    return true;
}

void HoymilesHMS::removeInverter(uint64_t serialNumber) {
    // Find and remove inverter
    for (uint8_t i = 0; i < m_inverterCount; i++) {
        if (m_inverters[i] == serialNumber) {
            // Shift remaining inverters
            for (uint8_t j = i; j < m_inverterCount - 1; j++) {
                m_inverters[j] = m_inverters[j + 1];
            }
            m_inverters[m_inverterCount - 1] = 0;
            m_inverterCount--;

            DEBUG_PRINT("Hoymiles HMS/HMT: Removed inverter - Serial: ");
            DEBUG_PRINTLN((unsigned long)(serialNumber & 0xFFFFFFFF));
            return;
        }
    }

    DEBUG_PRINTLN("Hoymiles HMS/HMT: Inverter not found in list");
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
    if (m_inverterCount == 0) {
        DEBUG_PRINTLN("Hoymiles HMS/HMT: No inverters registered");
        return;
    }

    DEBUG_PRINT("Hoymiles HMS/HMT: Polling ");
    DEBUG_PRINT(m_inverterCount);
    DEBUG_PRINTLN(" inverter(s)...");

    // Poll each registered inverter
    for (uint8_t i = 0; i < m_inverterCount; i++) {
        uint64_t serialNumber = m_inverters[i];

        DEBUG_PRINT("  [");
        DEBUG_PRINT(i + 1);
        DEBUG_PRINT("/");
        DEBUG_PRINT(m_inverterCount);
        DEBUG_PRINT("] Serial: ");
        DEBUG_PRINTLN((unsigned long)(serialNumber & 0xFFFFFFFF));

        // Send request
        sendRequest(serialNumber);

        // Wait for response
        bool success = receiveResponse(serialNumber);

        if (success) {
            DEBUG_PRINTLN("    ✓ Response received and parsed");
        } else {
            DEBUG_PRINTLN("    ✗ No response or parse error");
        }

        // Small delay between inverters
        delay(100);
    }
}

void HoymilesHMS::sendRequest(uint64_t serialNumber) {
    if (!g_radio) {
        DEBUG_PRINTLN("Hoymiles HMS/HMT: ERROR - Radio not initialized");
        return;
    }

    // Build HMS realtime data request packet
    uint8_t packet[HOYMILES_PACKET_MAX_SIZE];
    uint8_t packetSize = HoymilesProtocol::buildHMSRealtimeRequest(
        packet, HOYMILES_DTU_SERIAL, serialNumber);

    DEBUG_PRINT("Hoymiles HMS/HMT: Sending request (");
    DEBUG_PRINT(packetSize);
    DEBUG_PRINT(" bytes) to inverter ");
    DEBUG_PRINTLN((unsigned long)(serialNumber & 0xFFFFFFFF));

    // Debug: Print packet
    DEBUG_PRINT("    Packet: ");
    for (uint8_t i = 0; i < packetSize; i++) {
        if (packet[i] < 0x10) DEBUG_PRINT("0");
        DEBUG_PRINT(packet[i], HEX);
        DEBUG_PRINT(" ");
    }
    DEBUG_PRINTLN();

    // Switch to transmit mode and send packet
    CMT2300A radio = new CMT2300A(g_radio);
    int state = radio.transmit(packet, packetSize);

    if (state == RADIOLIB_ERR_NONE) {
        DEBUG_PRINTLN("    Transmission successful");
    } else {
        DEBUG_PRINT("    ERROR - Transmission failed! Code: ");
        DEBUG_PRINTLN(state);
    }

    // Return to receive mode
    radio.startReceive();
}

bool HoymilesHMS::receiveResponse(uint64_t serialNumber) {
    if (!g_radio) {
        DEBUG_PRINTLN("Hoymiles HMS/HMT: ERROR - Radio not initialized");
        return false;
    }

    DEBUG_PRINTLN("Hoymiles HMS/HMT: Waiting for response...");

    CMT2300A radio = new CMT2300A(g_radio);

    // Wait for response with timeout
    unsigned long timeout = millis() + 1000; // 1 second timeout (HMS may take longer)
    uint8_t packet[HOYMILES_PACKET_MAX_SIZE];
    int packetLength = 0;

    while (millis() < timeout) {
        // Check if packet received
        packetLength = radio.getPacketLength();

        if (packetLength > 0) {
            DEBUG_PRINT("    Packet received (");
            DEBUG_PRINT(packetLength);
            DEBUG_PRINTLN(" bytes)");

            // Read packet
            int state = radio.readData(packet, packetLength);

            if (state != RADIOLIB_ERR_NONE) {
                DEBUG_PRINT("    ERROR - Failed to read packet! Code: ");
                DEBUG_PRINTLN(state);
                return false;
            }

            // Debug: Print packet
            DEBUG_PRINT("    Packet: ");
            for (int i = 0; i < packetLength; i++) {
                if (packet[i] < 0x10) DEBUG_PRINT("0");
                DEBUG_PRINT(packet[i], HEX);
                DEBUG_PRINT(" ");
            }
            DEBUG_PRINTLN();

            // Print RSSI and SNR
            DEBUG_PRINT("    RSSI: ");
            DEBUG_PRINT(radio.getRSSI());
            DEBUG_PRINT(" dBm, SNR: ");
            DEBUG_PRINT(radio.getSNR());
            DEBUG_PRINTLN(" dB");

            // Parse response
            float power, voltage, current, frequency, temperature;
            bool parseSuccess = HoymilesProtocol::parseHMSRealtimeResponse(
                packet, packetLength,
                power, voltage, current,
                frequency, temperature);

            if (parseSuccess) {
                DEBUG_PRINTLN("    Data parsed successfully:");
                DEBUG_PRINT("      Power: ");
                DEBUG_PRINT(power);
                DEBUG_PRINTLN(" W");
                DEBUG_PRINT("      Voltage: ");
                DEBUG_PRINT(voltage);
                DEBUG_PRINTLN(" V");
                DEBUG_PRINT("      Current: ");
                DEBUG_PRINT(current);
                DEBUG_PRINTLN(" A");
                DEBUG_PRINT("      Frequency: ");
                DEBUG_PRINT(frequency);
                DEBUG_PRINTLN(" Hz");
                DEBUG_PRINT("      Temperature: ");
                DEBUG_PRINT(temperature);
                DEBUG_PRINTLN(" °C");

                // Call data callback if set
                if (m_dataCallback) {
                    m_dataCallback(serialNumber, power, voltage, current);
                }

                // Return to receive mode
                radio.startReceive();
                return true;
            } else {
                DEBUG_PRINTLN("    ERROR - Failed to parse response (invalid CRC or format)");
            }
        }

        // Small delay
        delay(10);
    }

    DEBUG_PRINTLN("    Timeout - No response received");

    // Return to receive mode
    radio.startReceive();
    return false;
}

void HoymilesHMS::setDataCallback(std::function<void(uint64_t serial, float power, float voltage, float current)> callback) {
    m_dataCallback = callback;
    DEBUG_PRINTLN("Hoymiles HMS/HMT: Data callback registered");
}

#endif // RADIO_CMT2300A
