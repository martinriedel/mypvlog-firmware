/**
 * Hoymiles HM Protocol - NRF24L01+ Communication
 *
 * Implementation based on OpenDTU reverse-engineered protocol
 * Reference: https://github.com/tbnobody/OpenDTU
 */

#include "hoymiles_hm.h"
#include "config.h"

#ifdef RADIO_NRF24

HoymilesHM::HoymilesHM()
    : m_lastPoll(0)
    , m_pollInterval(HOYMILES_POLL_INTERVAL)
    , m_inverterCount(0)
    , m_radio(nullptr)
{
    // Initialize inverter list
    for (uint8_t i = 0; i < HOYMILES_MAX_INVERTERS; i++) {
        m_inverters[i] = 0;
    }
}

void HoymilesHM::begin() {
    DEBUG_PRINTLN("Hoymiles HM: Initializing...");

    // Create RF24 instance
    m_radio = new RF24(NRF24_CE_PIN, NRF24_CS_PIN);

    // Initialize radio
    if (!m_radio->begin()) {
        DEBUG_PRINTLN("Hoymiles HM: ERROR - NRF24 initialization failed!");
        return;
    }

    // Configure NRF24 for Hoymiles communication
    // Hoymiles uses channel 40 (2440 MHz)
    m_radio->setChannel(40);

    // Use 250kbps for better range (Hoymiles inverters use this)
    m_radio->setDataRate(RF24_250KBPS);

    // Maximum PA level for long range
    m_radio->setPALevel(RF24_PA_MAX);

    // Disable auto-acknowledgment (Hoymiles doesn't use it)
    m_radio->setAutoAck(false);

    // Use 16-bit CRC
    m_radio->setCRCLength(RF24_CRC_16);

    // Enable dynamic payloads
    m_radio->enableDynamicPayloads();

    // Set retry delay and count
    m_radio->setRetries(15, 15);  // Max delay, max retries

    // Open reading pipe 0 (for receiving responses)
    uint8_t rxAddress[5] = {0xCC, 0xCC, 0xCC, 0xCC, 0xCC};
    m_radio->openReadingPipe(0, rxAddress);

    // Start listening for responses
    m_radio->startListening();

    DEBUG_PRINTLN("Hoymiles HM: Initialized successfully");
    DEBUG_PRINT("  Channel: ");
    DEBUG_PRINTLN(m_radio->getChannel());
    DEBUG_PRINT("  Data Rate: ");
    DEBUG_PRINTLN(m_radio->getDataRate() == RF24_250KBPS ? "250kbps" : "Unknown");
    DEBUG_PRINT("  PA Level: ");
    DEBUG_PRINTLN(m_radio->getPALevel());
}

void HoymilesHM::loop() {
    if (!m_radio) {
        return;  // Not initialized
    }

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

    // Check if already exists
    for (uint8_t i = 0; i < m_inverterCount; i++) {
        if (m_inverters[i] == serialNumber) {
            DEBUG_PRINTLN("Hoymiles HM: Inverter already exists");
            return false;
        }
    }

    // Add to list
    m_inverters[m_inverterCount] = serialNumber;
    m_inverterCount++;

    DEBUG_PRINT("Hoymiles HM: Added inverter #");
    DEBUG_PRINT(m_inverterCount);
    DEBUG_PRINT(" with serial ");
    DEBUG_PRINTLN((unsigned long)(serialNumber & 0xFFFFFFFF));

    return true;
}

void HoymilesHM::removeInverter(uint64_t serialNumber) {
    // Find and remove
    for (uint8_t i = 0; i < m_inverterCount; i++) {
        if (m_inverters[i] == serialNumber) {
            // Shift remaining inverters down
            for (uint8_t j = i; j < m_inverterCount - 1; j++) {
                m_inverters[j] = m_inverters[j + 1];
            }
            m_inverters[m_inverterCount - 1] = 0;
            m_inverterCount--;

            DEBUG_PRINT("Hoymiles HM: Removed inverter ");
            DEBUG_PRINTLN((unsigned long)(serialNumber & 0xFFFFFFFF));
            return;
        }
    }

    DEBUG_PRINTLN("Hoymiles HM: Inverter not found");
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
    if (!m_radio || m_inverterCount == 0) {
        return;
    }

    DEBUG_PRINT("Hoymiles HM: Polling ");
    DEBUG_PRINT(m_inverterCount);
    DEBUG_PRINTLN(" inverter(s)...");

    // Poll each inverter
    for (uint8_t i = 0; i < m_inverterCount; i++) {
        uint64_t serial = m_inverters[i];

        DEBUG_PRINT("  [");
        DEBUG_PRINT(i + 1);
        DEBUG_PRINT("/");
        DEBUG_PRINT(m_inverterCount);
        DEBUG_PRINT("] Polling ");
        DEBUG_PRINTLN((unsigned long)(serial & 0xFFFFFFFF));

        // Send request
        sendRequest(serial);

        // Wait for response
        if (receiveResponse(serial)) {
            DEBUG_PRINTLN("    Success!");
        } else {
            DEBUG_PRINTLN("    Timeout/No response");
        }

        // Small delay between inverters
        delay(50);
    }
}

void HoymilesHM::sendRequest(uint64_t serialNumber) {
    if (!m_radio) {
        return;
    }

    // Build request packet
    uint8_t packet[HOYMILES_PACKET_MAX_SIZE];
    uint8_t packetSize = HoymilesProtocol::buildRealtimeRequest(
        packet,
        HOYMILES_DTU_SERIAL,
        serialNumber
    );

    // Convert serial to NRF24 address
    uint8_t inverterAddress[5];
    HoymilesProtocol::serialToAddress(serialNumber, inverterAddress);

    DEBUG_PRINT("    TX Address: ");
    for (int i = 0; i < 5; i++) {
        if (inverterAddress[i] < 0x10) DEBUG_PRINT("0");
        DEBUG_PRINT(inverterAddress[i], HEX);
        if (i < 4) DEBUG_PRINT(":");
    }
    DEBUG_PRINTLN();

    DEBUG_PRINT("    TX Packet (");
    DEBUG_PRINT(packetSize);
    DEBUG_PRINT(" bytes): ");
    for (uint8_t i = 0; i < packetSize; i++) {
        if (packet[i] < 0x10) DEBUG_PRINT("0");
        DEBUG_PRINT(packet[i], HEX);
        DEBUG_PRINT(" ");
    }
    DEBUG_PRINTLN();

    // Stop listening, configure for transmission
    m_radio->stopListening();

    // Set inverter address for transmission
    m_radio->openWritingPipe(inverterAddress);

    // Send packet
    bool success = m_radio->write(packet, packetSize);

    if (success) {
        DEBUG_PRINTLN("    TX: Packet sent successfully");
    } else {
        DEBUG_PRINTLN("    TX: Failed to send packet");
    }

    // Switch back to listening mode
    m_radio->startListening();
}

bool HoymilesHM::receiveResponse(uint64_t serialNumber) {
    if (!m_radio) {
        return false;
    }

    unsigned long timeout = millis() + 500;  // 500ms timeout

    while (millis() < timeout) {
        if (m_radio->available()) {
            uint8_t packet[HOYMILES_PACKET_MAX_SIZE];
            uint8_t len = m_radio->getDynamicPayloadSize();

            if (len > HOYMILES_PACKET_MAX_SIZE) {
                DEBUG_PRINTLN("    RX: Packet too large!");
                return false;
            }

            // Read packet
            m_radio->read(packet, len);

            DEBUG_PRINT("    RX Packet (");
            DEBUG_PRINT(len);
            DEBUG_PRINT(" bytes): ");
            for (uint8_t i = 0; i < len; i++) {
                if (packet[i] < 0x10) DEBUG_PRINT("0");
                DEBUG_PRINT(packet[i], HEX);
                DEBUG_PRINT(" ");
            }
            DEBUG_PRINTLN();

            // Parse response
            float power, voltage, current, frequency, temperature;
            if (HoymilesProtocol::parseRealtimeResponse(packet, len,
                                                       power, voltage, current,
                                                       frequency, temperature)) {
                DEBUG_PRINT("    Power: ");
                DEBUG_PRINT(power);
                DEBUG_PRINTLN(" W");

                DEBUG_PRINT("    Voltage: ");
                DEBUG_PRINT(voltage);
                DEBUG_PRINTLN(" V");

                DEBUG_PRINT("    Current: ");
                DEBUG_PRINT(current);
                DEBUG_PRINTLN(" A");

                DEBUG_PRINT("    Frequency: ");
                DEBUG_PRINT(frequency);
                DEBUG_PRINTLN(" Hz");

                DEBUG_PRINT("    Temperature: ");
                DEBUG_PRINT(temperature);
                DEBUG_PRINTLN(" °C");

                // Call callback if set
                if (m_dataCallback) {
                    m_dataCallback(serialNumber, power, voltage, current);
                }

                return true;
            } else {
                DEBUG_PRINTLN("    RX: Invalid packet or CRC error");
            }
        }

        // Small delay to avoid busy-waiting
        delay(10);
    }

    return false;
}

void HoymilesHM::setDataCallback(std::function<void(uint64_t serial, float power, float voltage, float current)> callback) {
    m_dataCallback = callback;
}

#endif // RADIO_NRF24
