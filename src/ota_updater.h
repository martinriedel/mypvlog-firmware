/**
 * OTA (Over-The-Air) Firmware Updater
 *
 * Handles downloading and installing firmware updates from mypvlog.net
 */

#ifndef OTA_UPDATER_H
#define OTA_UPDATER_H

#include <Arduino.h>
#include <functional>

// Update status
enum class OTAStatus {
    IDLE,
    CHECKING,
    DOWNLOADING,
    INSTALLING,
    SUCCESS,
    FAILED
};

// Update progress callback
// Parameters: status, progress (0-100), message
typedef std::function<void(OTAStatus status, int progress, const String& message)> OTAProgressCallback;

class OTAUpdater {
public:
    OTAUpdater();

    /**
     * Check and perform firmware update
     *
     * @param downloadUrl URL to download firmware from
     * @param expectedChecksum MD5 checksum for verification
     * @param progressCallback Called during download/install
     * @return true if update successful
     */
    bool performUpdate(const String& downloadUrl,
                      const String& expectedChecksum,
                      OTAProgressCallback progressCallback = nullptr);

    /**
     * Get last error message
     */
    String getLastError() const { return m_lastError; }

    /**
     * Get current status
     */
    OTAStatus getStatus() const { return m_status; }

private:
    OTAStatus m_status;
    String m_lastError;
    OTAProgressCallback m_progressCallback;

    void setStatus(OTAStatus status, int progress, const String& message);
    bool verifyChecksum(const String& expected);
};

#endif // OTA_UPDATER_H
