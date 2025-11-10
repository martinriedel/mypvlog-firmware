/**
 * Hoymiles Protocol - Common protocol functions
 *
 * Supports both HM series (NRF24L01+) and HMS/HMT series (CMT2300A)
 * Based on reverse-engineering work from OpenDTU project
 */

#ifndef HOYMILES_PROTOCOL_H
#define HOYMILES_PROTOCOL_H

#include <Arduino.h>

// Protocol constants
#define HOYMILES_PACKET_MAX_SIZE    64
#define HOYMILES_DTU_SERIAL         99978563001ULL  // Default DTU serial (can be customized)

// Command types - HM Series (2.4GHz)
#define CMD_GET_REALTIME_DATA       0x0B
#define RESP_REALTIME_DATA          0x8B
#define CMD_GET_DEVICE_INFO         0x09
#define RESP_DEVICE_INFO            0x89

// Command types - HMS/HMT Series (868MHz)
#define HMS_CMD_GET_REALTIME_DATA   0x11
#define HMS_RESP_REALTIME_DATA      0x91
#define HMS_CMD_GET_DEVICE_INFO     0x15
#define HMS_RESP_DEVICE_INFO        0x95

class HoymilesProtocol {
public:
    /**
     * Calculate CRC8 checksum (polynomial 0x01)
     * Used for packet integrity checking
     */
    static uint8_t crc8(const uint8_t* data, uint8_t len) {
        uint8_t crc = 0;
        for (uint8_t i = 0; i < len; i++) {
            crc ^= data[i];
            for (uint8_t j = 0; j < 8; j++) {
                if (crc & 0x01) {
                    crc = (crc >> 1) ^ 0x8C;
                } else {
                    crc >>= 1;
                }
            }
        }
        return crc;
    }

    /**
     * Convert inverter serial number to NRF24 address (HM series)
     *
     * @param serial Inverter serial number (e.g., 112182123456)
     * @param address Output buffer for 5-byte address
     */
    static void serialToAddress(uint64_t serial, uint8_t* address) {
        address[0] = (serial >> 32) & 0xFF;
        address[1] = (serial >> 24) & 0xFF;
        address[2] = (serial >> 16) & 0xFF;
        address[3] = (serial >> 8) & 0xFF;
        address[4] = serial & 0xFF;
    }

    /**
     * Build realtime data request packet for HM series (NRF24)
     *
     * Packet structure:
     * [0-1]   : Time counter (increments with each request)
     * [2]     : Command (0x0B for realtime data)
     * [3-6]   : DTU serial number (4 bytes)
     * [7-10]  : Inverter serial number (4 bytes)
     * [11]    : CRC8 checksum
     *
     * @param packet Output buffer (minimum 12 bytes)
     * @param dtuSerial DTU serial number
     * @param inverterSerial Inverter serial number
     * @return Packet size (always 12)
     */
    static uint8_t buildRealtimeRequest(uint8_t* packet, uint64_t dtuSerial, uint64_t inverterSerial) {
        static uint16_t timeCounter = 0;

        // Time counter (2 bytes)
        packet[0] = (timeCounter >> 8) & 0xFF;
        packet[1] = timeCounter & 0xFF;
        timeCounter++;

        // Command
        packet[2] = CMD_GET_REALTIME_DATA;

        // DTU serial (4 bytes, MSB first)
        packet[3] = (dtuSerial >> 24) & 0xFF;
        packet[4] = (dtuSerial >> 16) & 0xFF;
        packet[5] = (dtuSerial >> 8) & 0xFF;
        packet[6] = dtuSerial & 0xFF;

        // Inverter serial (4 bytes, MSB first)
        packet[7] = (inverterSerial >> 24) & 0xFF;
        packet[8] = (inverterSerial >> 16) & 0xFF;
        packet[9] = (inverterSerial >> 8) & 0xFF;
        packet[10] = inverterSerial & 0xFF;

        // CRC8 checksum
        packet[11] = crc8(packet, 11);

        return 12;
    }

    /**
     * Parse realtime data response from HM series inverter
     *
     * Response structure (varies by inverter model):
     * [0-1]   : Time counter (echo from request)
     * [2]     : Response code (0x8B)
     * [3-6]   : Inverter serial number
     * [7-8]   : DC Power (W * 10)
     * [9-10]  : AC Power (W * 10)
     * [11-12] : DC Voltage (V * 10)
     * [13-14] : DC Current (A * 100)
     * [15-16] : AC Voltage (V * 10)
     * [17-18] : AC Frequency (Hz * 100)
     * [19-20] : Temperature (°C * 10)
     * [21-22] : Grid voltage (V * 10)
     * [...]   : Additional fields depending on model
     * [n-1]   : CRC8 checksum
     *
     * @param packet Response packet buffer
     * @param len Packet length
     * @param power Output: AC power in watts
     * @param voltage Output: AC voltage in volts
     * @param current Output: DC current in amps
     * @param frequency Output: AC frequency in Hz
     * @param temperature Output: Inverter temperature in °C
     * @return true if packet valid and parsed successfully
     */
    static bool parseRealtimeResponse(const uint8_t* packet, uint8_t len,
                                     float& power, float& voltage, float& current,
                                     float& frequency, float& temperature) {
        // Minimum packet size check
        if (len < 23) {
            return false;
        }

        // Verify response code
        if (packet[2] != RESP_REALTIME_DATA) {
            return false;
        }

        // Verify CRC
        uint8_t crc = crc8(packet, len - 1);
        if (crc != packet[len - 1]) {
            return false;
        }

        // Parse data fields
        power = ((packet[9] << 8) | packet[10]) / 10.0f;       // AC Power
        voltage = ((packet[15] << 8) | packet[16]) / 10.0f;    // AC Voltage
        current = ((packet[13] << 8) | packet[14]) / 100.0f;   // DC Current
        frequency = ((packet[17] << 8) | packet[18]) / 100.0f; // Frequency
        temperature = ((packet[19] << 8) | packet[20]) / 10.0f; // Temperature

        return true;
    }

    /**
     * Build realtime data request packet for HMS/HMT series (CMT2300A)
     *
     * HMS protocol differs slightly from HM:
     * - Uses 868MHz radio (CMT2300A)
     * - Different command codes
     * - Longer packets with more fields
     *
     * Packet structure:
     * [0-1]   : Time counter
     * [2]     : Command (0x11 for HMS realtime data)
     * [3-10]  : DTU serial number (8 bytes for HMS)
     * [11-18] : Inverter serial number (8 bytes)
     * [19]    : Packet counter
     * [20]    : CRC8 checksum
     *
     * @param packet Output buffer (minimum 21 bytes)
     * @param dtuSerial DTU serial number (full 64-bit)
     * @param inverterSerial Inverter serial number (full 64-bit)
     * @return Packet size (always 21)
     */
    static uint8_t buildHMSRealtimeRequest(uint8_t* packet, uint64_t dtuSerial, uint64_t inverterSerial) {
        static uint16_t timeCounter = 0;
        static uint8_t packetCounter = 0;

        // Time counter (2 bytes)
        packet[0] = (timeCounter >> 8) & 0xFF;
        packet[1] = timeCounter & 0xFF;
        timeCounter++;

        // Command for HMS
        packet[2] = HMS_CMD_GET_REALTIME_DATA;

        // DTU serial (8 bytes, MSB first)
        packet[3] = (dtuSerial >> 56) & 0xFF;
        packet[4] = (dtuSerial >> 48) & 0xFF;
        packet[5] = (dtuSerial >> 40) & 0xFF;
        packet[6] = (dtuSerial >> 32) & 0xFF;
        packet[7] = (dtuSerial >> 24) & 0xFF;
        packet[8] = (dtuSerial >> 16) & 0xFF;
        packet[9] = (dtuSerial >> 8) & 0xFF;
        packet[10] = dtuSerial & 0xFF;

        // Inverter serial (8 bytes, MSB first)
        packet[11] = (inverterSerial >> 56) & 0xFF;
        packet[12] = (inverterSerial >> 48) & 0xFF;
        packet[13] = (inverterSerial >> 40) & 0xFF;
        packet[14] = (inverterSerial >> 32) & 0xFF;
        packet[15] = (inverterSerial >> 24) & 0xFF;
        packet[16] = (inverterSerial >> 16) & 0xFF;
        packet[17] = (inverterSerial >> 8) & 0xFF;
        packet[18] = inverterSerial & 0xFF;

        // Packet counter
        packet[19] = packetCounter++;

        // CRC8 checksum
        packet[20] = crc8(packet, 20);

        return 21;
    }

    /**
     * Parse realtime data response from HMS/HMT series inverter
     *
     * HMS response structure (longer than HM):
     * [0-1]   : Time counter
     * [2]     : Response code (0x91)
     * [3-10]  : Inverter serial number (8 bytes)
     * [11-12] : DC Power Channel 1 (W * 10)
     * [13-14] : DC Power Channel 2 (W * 10)
     * [15-16] : AC Power (W * 10)
     * [17-18] : DC Voltage Channel 1 (V * 10)
     * [19-20] : DC Voltage Channel 2 (V * 10)
     * [21-22] : DC Current Channel 1 (A * 100)
     * [23-24] : DC Current Channel 2 (A * 100)
     * [25-26] : AC Voltage (V * 10)
     * [27-28] : AC Frequency (Hz * 100)
     * [29-30] : Temperature (°C * 10)
     * [...]   : Additional fields
     * [n-1]   : CRC8 checksum
     *
     * @param packet Response packet buffer
     * @param len Packet length
     * @param power Output: Total AC power in watts
     * @param voltage Output: AC voltage in volts
     * @param current Output: Total DC current in amps (sum of channels)
     * @param frequency Output: AC frequency in Hz
     * @param temperature Output: Inverter temperature in °C
     * @return true if packet valid and parsed successfully
     */
    static bool parseHMSRealtimeResponse(const uint8_t* packet, uint8_t len,
                                        float& power, float& voltage, float& current,
                                        float& frequency, float& temperature) {
        // Minimum packet size check (HMS packets are longer)
        if (len < 32) {
            return false;
        }

        // Verify response code
        if (packet[2] != HMS_RESP_REALTIME_DATA) {
            return false;
        }

        // Verify CRC
        uint8_t crc = crc8(packet, len - 1);
        if (crc != packet[len - 1]) {
            return false;
        }

        // Parse data fields
        power = ((packet[15] << 8) | packet[16]) / 10.0f;      // AC Power
        voltage = ((packet[25] << 8) | packet[26]) / 10.0f;    // AC Voltage

        // Sum DC current from both channels
        float current1 = ((packet[21] << 8) | packet[22]) / 100.0f;
        float current2 = ((packet[23] << 8) | packet[24]) / 100.0f;
        current = current1 + current2;

        frequency = ((packet[27] << 8) | packet[28]) / 100.0f; // Frequency
        temperature = ((packet[29] << 8) | packet[30]) / 10.0f; // Temperature

        return true;
    }
};

#endif // HOYMILES_PROTOCOL_H
