# Building and Flashing MyPVLog Firmware

Complete guide to building and flashing the firmware to your ESP32/ESP8266.

---

## Prerequisites

### Required Hardware

- **ESP32 Development Board** (recommended)
  - ESP32 DevKit v1 / ESP32-WROOM-32
  - ESP32-S2 / ESP32-S3
  - ESP32-C3
- **Or ESP8266 Development Board** (limited features)
  - NodeMCU v2/v3
  - Wemos D1 Mini
- **USB Cable** (data cable, not just power)
- **nRF24L01+ Module** (for Hoymiles HM support)
- **CMT2300A Module** (optional, for Hoymiles HMS/HMT support)

### Required Software

1. **PlatformIO** - Choose one:
   - [VS Code + PlatformIO Extension](https://platformio.org/platformio-ide) (recommended)
   - [PlatformIO CLI](https://docs.platformio.org/en/latest/core/installation/index.html)

2. **Git** (for cloning the repository)

3. **Python 3.x** (usually installed with PlatformIO)

---

## Quick Start

### 1. Clone the Repository

```bash
git clone https://github.com/martinriedel/mypvlog-firmware.git
cd mypvlog-firmware
```

### 2. Choose Your Firmware Variant

| Variant | Platform | Radio | Use Case |
|---------|----------|-------|----------|
| `esp32-nrf24` | ESP32 | NRF24L01+ | **Hoymiles HM only** ⭐ Most common |
| `esp32-dual` | ESP32 | NRF24 + CMT2300A | **HM + HMS/HMT** |
| `esp32s3-dual` | ESP32-S3 | NRF24 + CMT2300A | Latest hardware |
| `esp8266-nrf24` | ESP8266 | NRF24L01+ | Budget option (limited) |

### 3. Build the Firmware

```bash
# Build specific variant
pio run -e esp32-nrf24

# Or build all variants
pio run
```

### 4. Upload to Device

```bash
# Connect ESP32 via USB, then upload
pio run -e esp32-nrf24 -t upload

# Monitor serial output
pio device monitor -b 115200
```

---

## Detailed Build Instructions

### Option 1: Using VS Code + PlatformIO Extension

1. **Install VS Code**
   - Download from https://code.visualstudio.com/

2. **Install PlatformIO Extension**
   - Open VS Code
   - Go to Extensions (Ctrl+Shift+X)
   - Search for "PlatformIO IDE"
   - Click Install

3. **Open Project**
   - File → Open Folder
   - Select `mypvlog-firmware` directory

4. **Build & Upload**
   - Click PlatformIO icon in sidebar
   - Select your environment (e.g., `esp32-nrf24`)
   - Click "Build" or "Upload"

5. **Monitor Serial**
   - Click "Serial Monitor" in PlatformIO panel
   - Baud rate: 115200

### Option 2: Using PlatformIO CLI

1. **Install PlatformIO Core**
   ```bash
   pip install platformio
   ```

2. **Build Firmware**
   ```bash
   cd mypvlog-firmware
   pio run -e esp32-nrf24
   ```

3. **Upload to Device**
   ```bash
   # Auto-detect port
   pio run -e esp32-nrf24 -t upload

   # Or specify port manually
   pio run -e esp32-nrf24 -t upload --upload-port /dev/ttyUSB0  # Linux
   pio run -e esp32-nrf24 -t upload --upload-port COM3           # Windows
   ```

4. **Monitor Serial Output**
   ```bash
   pio device monitor -b 115200
   ```

---

## Uploading Web UI Files (LittleFS)

The web UI files need to be uploaded to the ESP32's filesystem.

### Using PlatformIO

```bash
# Upload filesystem (web UI files from data/ folder)
pio run -e esp32-nrf24 -t uploadfs

# Then upload firmware
pio run -e esp32-nrf24 -t upload
```

**Note:** The first time you flash, you MUST upload the filesystem!

---

## Troubleshooting

### Upload Fails: "Failed to connect"

**Solution:** Put ESP32 in boot mode manually
1. Hold down BOOT button
2. Press and release RESET button
3. Release BOOT button
4. Try upload again

### Port Not Found

**Linux:**
```bash
# Add user to dialout group
sudo usermod -a -G dialout $USER
# Log out and log back in

# Check port
ls /dev/ttyUSB*  # or /dev/ttyACM*
```

**Windows:**
- Install [CP2102 driver](https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers)
- Check Device Manager for COM port number

**macOS:**
```bash
# Install driver if needed
brew install --cask silicon-labs-vcp-driver

# Check port
ls /dev/cu.usb*
```

### Out of Memory / Build Fails

**Solution:** Reduce build variant or increase partition size
- ESP32 has 4MB flash minimum
- ESP8266 needs `board_build.ldscript = eagle.flash.4m2m.ld`

### Web UI Not Loading

**Causes:**
1. Filesystem not uploaded → Run `pio run -e esp32-nrf24 -t uploadfs`
2. LittleFS mount failed → Check serial output for errors
3. Wrong partition table → Verify `board_build.partitions` in platformio.ini

---

## Firmware Customization

### Changing WiFi AP Credentials

Edit `src/config.h`:
```cpp
#define WIFI_AP_SSID_PREFIX "MyPVLog-"
#define WIFI_AP_PASSWORD "mypvlog123"  // Change this
```

### Changing Debug Level

Edit `src/config.h`:
```cpp
#define DEBUG_ENABLED true  // Set to false to disable debug output
```

### Changing Poll Interval

Edit `src/config.h`:
```cpp
#define HOYMILES_POLL_INTERVAL 5000      // Generic mode: 5 seconds
#define HOYMILES_POLL_INTERVAL_FAST 2000 // MyPVLog Direct: 2 seconds
```

---

## Wiring Diagrams

### ESP32 + NRF24L01+

```
ESP32 GPIO    →    NRF24L01+
----------------------------------------
GPIO 5  (CS)  →    CSN
GPIO 18 (SCK) →    SCK
GPIO 19 (MISO)→    MISO
GPIO 23 (MOSI)→    MOSI
GPIO 2        →    CE
GPIO 16       →    IRQ (optional)
3.3V          →    VCC
GND           →    GND
```

**Important:**
- Use **3.3V**, not 5V! (NRF24 is 3.3V only)
- Add 10µF capacitor between VCC and GND (close to NRF24)
- Use short wires (< 10cm) for stable SPI communication

### ESP32 + CMT2300A (for HMS/HMT)

```
ESP32 GPIO    →    CMT2300A
----------------------------------------
GPIO 15       →    CS
GPIO 18 (SCK) →    SCK (shared with NRF24)
GPIO 19 (MISO)→    MISO (shared)
GPIO 23 (MOSI)→    MOSI (shared)
GPIO 4        →    FCSB
3.3V          →    VCC
GND           →    GND
```

---

## OTA (Over-The-Air) Updates

Once the firmware is running and connected to WiFi, you can update wirelessly.

### Using PlatformIO

```bash
# Upload via OTA (device must be on same network)
pio run -e esp32-nrf24 -t upload --upload-port mypvlog-XXXX.local
```

### Using Web UI

1. Go to http://[device-ip]/
2. Navigate to System → Firmware Update
3. Select `.bin` file from `.pio/build/esp32-nrf24/firmware.bin`
4. Click Upload
5. Device reboots with new firmware

---

## Factory Reset

### Via Web UI
1. Go to http://[device-ip]/api/system/factory-reset
2. Or use the reset button in the web interface

### Via Serial
1. Connect to serial monitor
2. Send command: `reset factory`

### Via Hardware
1. Short GPIO 0 to GND during boot
2. Device enters safe mode and clears all settings

---

## Serial Monitor Output Example

Successful boot should look like:

```
========================================
  MyPVLog Firmware v1.0.0
  Build: Nov 08 2025 12:00:00
========================================
Platform: ESP32
Radio: NRF24L01+ (Hoymiles HM)

Operation Mode: Not Configured (Setup Required)

WiFi Manager: No saved credentials, starting AP mode
WiFi Manager: AP started successfully!
WiFi Manager: SSID: MyPVLog-ABCD
WiFi Manager: Password: mypvlog123
WiFi Manager: IP Address: 192.168.4.1

Web Server: LittleFS mounted
Web Server: DNS server started for captive portal
Web Server: Routes configured
Web Server: Started on port 80

========================================
  SETUP MODE
========================================
  Connect to WiFi: MyPVLog-ABCD
  Password: mypvlog123
  Then open: http://192.168.4.1
========================================

Initialization complete!
```

---

## Next Steps

After successfully flashing:

1. **Connect to AP**
   - WiFi network: `MyPVLog-XXXX`
   - Password: `mypvlog123`

2. **Open Web Browser**
   - URL: http://192.168.4.1
   - Captive portal should auto-open

3. **Complete Setup Wizard**
   - Select operation mode (Generic MQTT or MyPVLog Direct)
   - Configure WiFi
   - Configure MQTT or login to mypvlog.net

4. **Wire Up Inverters**
   - Connect NRF24L01+ module
   - Test communication with Hoymiles inverter

---

## Getting Help

- **GitHub Issues:** https://github.com/martinriedel/mypvlog-firmware/issues
- **Documentation:** https://docs.mypvlog.net
- **Community Forum:** https://github.com/martinriedel/mypvlog-firmware/discussions

---

## Advanced: Building for Multiple Variants

```bash
# Build all variants in parallel
pio run

# Check firmware sizes
pio run -t size

# Clean build artifacts
pio run -t clean

# Run tests
pio test
```

---

**Happy Building!** ☀️
