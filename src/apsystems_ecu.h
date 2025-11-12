/**
 * APSystems ECU-R Client - TCP/IP Communication
 *
 * Supports:
 * - APSystems YC600, YC1000, QT2 microinverters
 * - APSystems QS1, DS3 microinverters
 * - APSystems ECU-B, ECU-R, ECU-C gateways
 *
 * Communication: TCP port 8899 (binary protocol)
 *
 * This implementation communicates with an APSystems ECU gateway device
 * over the local network. The ECU acts as a bridge to the inverters using
 * Zigbee wireless communication.
 *
 * Note: You need an APSystems ECU device with a static IP address on your LAN.
 */

#ifndef APSYSTEMS_ECU_H
#define APSYSTEMS_ECU_H

#include <Arduino.h>
#include <functional>

#ifdef APSYSTEMS_ECU

#include <WiFi.h>

#define APSYSTEMS_ECU_PORT 8899
#define APSYSTEMS_MAX_INVERTERS 8
#define APSYSTEMS_POLL_INTERVAL 5000
#define APSYSTEMS_SOCKET_TIMEOUT 10000

// APSystems inverter data structure
struct APSystemsInverterData {
    char uid[13];           // Inverter UID (12 chars + null)
    bool online;            // Online status
    float power[4];         // Power per channel (W)
    float voltage[4];       // Voltage per channel (V)
    float frequency;        // Grid frequency (Hz)
    float temperature;      // Inverter temperature (°C)
    uint8_t signalStrength; // Signal strength (0-100%)
    uint8_t channelCount;   // Number of channels (2 or 4)
};

class APSystemsECU {
public:
    APSystemsECU();
    ~APSystemsECU();

    // Initialize with ECU IP address
    bool begin(const char* ecuIP);
    void loop();

    // Configuration
    void setPollInterval(uint16_t interval);
    void setECUID(const char* ecuID);

    // Callback for inverter data
    // Note: For APSystems, power/voltage are per-channel, callback is invoked once per inverter
    void setDataCallback(std::function<void(const char* uid, const APSystemsInverterData& data)> callback);

    // Status
    bool isConnected();
    uint8_t getInverterCount();
    const char* getECUID();
    float getTotalPower();
    float getLifetimeEnergy();
    float getTodayEnergy();

private:
    // Connection settings
    char m_ecuIP[16];           // ECU IP address
    char m_ecuID[16];           // ECU ID (from initial query)
    bool m_initialized;

    // Polling
    unsigned long m_lastPoll;
    uint16_t m_pollInterval;

    // Inverter data
    uint8_t m_inverterCount;
    APSystemsInverterData m_inverters[APSYSTEMS_MAX_INVERTERS];

    // Statistics
    float m_totalPower;
    float m_lifetimeEnergy;
    float m_todayEnergy;

    // Callback
    std::function<void(const char* uid, const APSystemsInverterData& data)> m_dataCallback;

    // Protocol methods
    bool queryECU();
    bool queryInverters();
    bool querySignalStrength();
    bool sendCommand(const char* command, uint8_t* response, size_t& responseLen);

    // Parsing methods
    bool parseECUResponse(const uint8_t* data, size_t len);
    bool parseInverterResponse(const uint8_t* data, size_t len);
    bool parseSignalResponse(const uint8_t* data, size_t len);

    // Utility methods
    uint32_t apsIntFromBytes(const uint8_t* data, uint8_t offset, uint8_t len);
    bool validateChecksum(const uint8_t* data, size_t len);
};

#endif // APSYSTEMS_ECU

#endif // APSYSTEMS_ECU_H
