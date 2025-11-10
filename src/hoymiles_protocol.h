/**
 * Hoymiles Protocol Packet Structures
 *
 * Based on OpenDTU reverse-engineered protocol
 * Reference: https://github.com/tbnobody/OpenDTU
 */

#ifndef HOYMILES_PROTOCOL_H
#define HOYMILES_PROTOCOL_H

#include <Arduino.h>

// ============================================
// Protocol Constants
// ============================================

#define HOYMILES_PACKET_MAX_SIZE    64
#define HOYMILES_DTU_SERIAL         99978563001ULL  // Default DTU serial

// Command IDs
#define CMD_GET_REALTIME_DATA       0x0B
#define CMD_GET_ALARM_INFO          0x11
#define CMD_GET_SYSTEM_INFO         0x01

// Response IDs
#define RESP_REALTIME_DATA          0x8B
#define RESP_ALARM_INFO             0x91
#define RESP_SYSTEM_INFO            0x81

// ============================================
// CRC Functions
// ============================================

class HoymilesProtocol {
public:
    /**
     * Calculate CRC8 checksum for Hoymiles protocol
     * Polynomial: 0x01 (x^8 + 1)
     */
    static uint8_t crc8(const uint8_t* data, uint8_t len) {
        uint8_t crc = 0;
        for (uint8_t i = 0; i < len; i++) {
            crc ^= data[i];
            for (uint8_t j = 0; j < 8; j++) {
                if (crc & 0x80) {
                    crc = (crc << 1) ^ 0x01;
                } else {
                    crc <<= 1;
                }
            }
        }
        return crc;
    }

    /**
     * Convert serial number to NRF24 address
     * Hoymiles uses last 4 bytes of serial + 0x01 as address
     */
    static void serialToAddress(uint64_t serial, uint8_t* address) {
        address[0] = 0x01;
        address[1] = (serial >> 24) & 0xFF;
        address[2] = (serial >> 16) & 0xFF;
        address[3] = (serial >> 8) & 0xFF;
        address[4] = serial & 0xFF;
    }

    /**
     * Build real-time data request packet
     */
    static uint8_t buildRealtimeRequest(uint8_t* packet, uint64_t dtuSerial, uint64_t inverterSerial) {
        uint8_t pos = 0;

        // Packet start (2 bytes: 0x15 0x21 for request)
        packet[pos++] = 0x15;
        packet[pos++] = CMD_GET_REALTIME_DATA;

        // DTU serial (4 bytes)
        packet[pos++] = (dtuSerial >> 24) & 0xFF;
        packet[pos++] = (dtuSerial >> 16) & 0xFF;
        packet[pos++] = (dtuSerial >> 8) & 0xFF;
        packet[pos++] = dtuSerial & 0xFF;

        // Inverter serial (4 bytes)
        packet[pos++] = (inverterSerial >> 24) & 0xFF;
        packet[pos++] = (inverterSerial >> 16) & 0xFF;
        packet[pos++] = (inverterSerial >> 8) & 0xFF;
        packet[pos++] = inverterSerial & 0xFF;

        // Timestamp (4 bytes) - unix timestamp
        uint32_t timestamp = millis() / 1000;
        packet[pos++] = (timestamp >> 24) & 0xFF;
        packet[pos++] = (timestamp >> 16) & 0xFF;
        packet[pos++] = (timestamp >> 8) & 0xFF;
        packet[pos++] = timestamp & 0xFF;

        // CRC8 checksum
        packet[pos] = crc8(packet, pos);
        pos++;

        return pos;
    }

    /**
     * Parse real-time data response
     * Returns true if valid packet
     */
    static bool parseRealtimeResponse(const uint8_t* packet, uint8_t len,
                                     float& power, float& voltage, float& current,
                                     float& frequency, float& temperature) {
        // Minimum packet size check
        if (len < 20) {
            return false;
        }

        // Check response ID
        if (packet[1] != RESP_REALTIME_DATA) {
            return false;
        }

        // Validate CRC
        uint8_t crc = crc8(packet, len - 1);
        if (crc != packet[len - 1]) {
            return false;
        }

        // Parse data fields (varies by inverter model)
        // HM-series typical packet structure:
        // [0-1]: Header (0x95 0x8B)
        // [2-9]: Inverter info
        // [10-11]: AC Power (0.1W units)
        // [12-13]: AC Voltage (0.1V units)
        // [14-15]: AC Current (0.01A units)
        // [16-17]: Grid Frequency (0.01Hz units)
        // [18-19]: Temperature (0.1°C units)
        // [len-1]: CRC8

        if (len >= 20) {
            // AC Power (watts)
            uint16_t powerRaw = (packet[10] << 8) | packet[11];
            power = powerRaw * 0.1f;

            // AC Voltage (volts)
            uint16_t voltageRaw = (packet[12] << 8) | packet[13];
            voltage = voltageRaw * 0.1f;

            // AC Current (amps)
            uint16_t currentRaw = (packet[14] << 8) | packet[15];
            current = currentRaw * 0.01f;

            // Grid Frequency (Hz)
            uint16_t freqRaw = (packet[16] << 8) | packet[17];
            frequency = freqRaw * 0.01f;

            // Temperature (celsius)
            uint16_t tempRaw = (packet[18] << 8) | packet[19];
            temperature = tempRaw * 0.1f;

            return true;
        }

        return false;
    }
};

#endif // HOYMILES_PROTOCOL_H
