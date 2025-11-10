/**
 * mypvlog.net API Client
 *
 * Handles communication with mypvlog.net backend API
 * - Device provisioning
 * - Heartbeat updates
 * - Firmware update checks
 * - Inverter data publishing
 */

#ifndef MYPVLOG_API_H
#define MYPVLOG_API_H

#include <Arduino.h>
#include <functional>

// Provision response structure
struct ProvisionResponse {
    bool success;
    String error;
    String dtuId;
    String mqttUsername;
    String mqttPassword;
    String mqttBroker;
    int mqttPort;
    bool mqttUseSsl;
    String mqttTopicPrefix;
};

// Firmware update info
struct FirmwareUpdateInfo {
    bool updateAvailable;
    String version;
    String downloadUrl;
    String releaseNotes;
    long fileSizeBytes;
    String checksum;
};

// Heartbeat response
struct HeartbeatResponse {
    bool success;
    String error;
    bool configChanged;  // If true, device should re-provision
};

class MypvlogAPI {
public:
    MypvlogAPI();

    /**
     * Initialize API client
     * @param apiUrl Base URL for API (e.g., "https://api.mypvlog.net")
     */
    void begin(const String& apiUrl);

    /**
     * Set JWT token for authenticated requests (provisioning)
     * @param token JWT token from user login
     */
    void setAuthToken(const String& token);

    /**
     * Provision device with mypvlog.net backend
     * Requires JWT token to be set first
     *
     * @param deviceMac MAC address of the device
     * @param firmwareVersion Current firmware version
     * @param hardwareModel Hardware model (e.g., "esp32-nrf24")
     * @return ProvisionResponse with MQTT credentials or error
     */
    ProvisionResponse provision(const String& deviceMac,
                               const String& firmwareVersion,
                               const String& hardwareModel);

    /**
     * Send heartbeat to backend
     * Updates device status and metrics
     *
     * @param dtuId DTU ID from provisioning
     * @param mqttPassword MQTT password for authentication
     * @param uptime Uptime in seconds
     * @param freeHeap Free heap memory in bytes
     * @param rssi WiFi signal strength in dBm
     * @param ipAddress Current IP address
     * @return HeartbeatResponse
     */
    HeartbeatResponse sendHeartbeat(const String& dtuId,
                                   const String& mqttPassword,
                                   unsigned long uptime,
                                   uint32_t freeHeap,
                                   int rssi,
                                   const String& ipAddress);

    /**
     * Check for firmware updates
     *
     * @param currentVersion Current firmware version
     * @param hardwareModel Hardware model
     * @return FirmwareUpdateInfo with update details if available
     */
    FirmwareUpdateInfo checkFirmwareUpdate(const String& currentVersion,
                                          const String& hardwareModel);

    /**
     * Set callback for HTTP errors
     * Called when HTTP requests fail
     */
    void setErrorCallback(std::function<void(const String& error)> callback);

private:
    String m_apiUrl;
    String m_authToken;
    std::function<void(const String& error)> m_errorCallback;

    // Helper methods
    String makeGetRequest(const String& endpoint, const String& queryParams = "");
    String makePostRequest(const String& endpoint, const String& body, bool authenticated = false);
    String urlEncode(const String& str);
    bool parseJsonResponse(const String& response, String& error);
};

#endif // MYPVLOG_API_H
