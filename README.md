# mypvlog Firmware

[![Build Firmware](https://github.com/mypvlog/firmware/actions/workflows/build.yml/badge.svg)](https://github.com/mypvlog/firmware/actions/workflows/build.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![PlatformIO](https://img.shields.io/badge/PlatformIO-Community-orange)](https://platformio.org)

**Easy-to-flash ESP32/ESP8266 firmware for solar inverter monitoring with direct mypvlog.net integration.**

Supports **Hoymiles, TSUN, and APSystems** inverters. Flash your ESP32 in 2 minutes from your browser. Choose **Generic MQTT mode** for flexibility, or **mypvlog Direct mode** for zero-config cloud integration.

---

## ✨ Features

### Two Modes, One Firmware

**🔧 Generic MQTT Mode** (OpenDTU/AhoyDTU Compatible)
- Works with **any MQTT broker**
- OpenDTU-compatible topic structure
- Self-hosted, full control
- Perfect for Home Assistant, Node-RED, etc.

**☁️ mypvlog Direct Mode** (Optimized for mypvlog.net)
- **2-minute setup** with zero configuration
- Automatic device registration
- Cloud dashboard with historical data
- Mobile app integration
- Premium features: Fast polling, analytics, API access

### Performance

| Feature | OpenDTU/AhoyDTU | mypvlog Firmware |
|---------|-----------------|------------------|
| Setup time | 15-30 minutes | **2 minutes** (Direct mode) |
| Poll interval | 5-15 seconds | **2-5 seconds** |
| Web flasher | ❌ | ✅ **Browser-based** |
| Cloud integration | Manual setup | ✅ **Automatic** |
| OTA updates | Manual | ✅ **Automatic** |

---

## 🚀 Quick Start

### Option 1: Web Flasher (Recommended)

1. Go to **[flash.mypvlog.net](https://flash.mypvlog.net)** (coming soon)
2. Connect your ESP32/ESP8266 via USB
3. Click "Flash Firmware"
4. Wait 2 minutes
5. Connect to WiFi and choose your mode

**No software installation required!** Works in Chrome, Edge, and Opera.

### Option 2: Manual Flash (Advanced Users)

#### Using PlatformIO
```bash
# Clone repository
git clone https://github.com/mypvlog/firmware.git
cd firmware

# Build for ESP32 with NRF24
pio run -e esp32-nrf24

# Upload to device
pio run -e esp32-nrf24 -t upload
```

#### Using esptool.py
```bash
# Install esptool
pip install esptool

# Download latest firmware
wget https://github.com/mypvlog/firmware/releases/latest/download/esp32-nrf24.bin

# Flash to ESP32
esptool.py --port /dev/ttyUSB0 write_flash 0x0 esp32-nrf24.bin
```

---

## 🛠️ Supported Hardware

### ESP Boards

| Board | Support | Flash Required | Recommended |
|-------|---------|----------------|-------------|
| ESP32 DevKit v1 | ✅ Full | 4MB | ⭐ Yes |
| ESP32-S2/S3 | ✅ Full | 4MB | ⭐ Yes (faster WiFi) |
| ESP32-C3 | ✅ Full | 4MB | ✅ Yes (low cost) |
| ESP8266 NodeMCU | ⚠️ Basic | 4MB | ⚠️ Limited (no SSL in Generic mode) |

### Radio Modules

**For Hoymiles HM Series (300-1500W):**
- nRF24L01+ (standard range, ~100m)
- nRF24L01+PA+LNA (long range, ~1000m) ⭐ **Recommended**

**For Hoymiles HMS/HMT Series (800-2000W):**
- CMT2300A module (433/868/915MHz)

### Compatible DTU Boards

Pre-built boards with ESP32 + radio modules:
- **OpenDTU Fusion Board** - ESP32 + NRF24 + CMT2300A (~€30)
- **AhoyDTU PCB** - ESP32 + NRF24 (~€25)
- **DIY Breadboard** - ESP32 + modules (~€15)

### Wiring

**ESP32 + NRF24L01+:**
```
ESP32 GPIO    →    NRF24L01+
----------------------------------------
GPIO 5  (VSPI CS)   →    CSN
GPIO 18 (VSPI SCK)  →    SCK
GPIO 19 (VSPI MISO) →    MISO
GPIO 23 (VSPI MOSI) →    MOSI
GPIO 2              →    CE
GPIO 16             →    IRQ (optional)
3.3V                →    VCC
GND                 →    GND
```

**ESP32 + CMT2300A (for HMS/HMT):**
```
ESP32 GPIO    →    CMT2300A
----------------------------------------
GPIO 15             →    CS
GPIO 18 (shared)    →    SCK
GPIO 19 (shared)    →    MISO
GPIO 23 (shared)    →    MOSI
GPIO 4              →    FCSB
3.3V                →    VCC
GND                 →    GND
```

---

## 📖 Setup Guide

### First Boot: Captive Portal

1. **Power on** your ESP32/ESP8266
2. **Connect** to WiFi AP: `mypvlog-XXXXXX` (password: `mypvlog123`)
3. **Open browser** - Captive portal opens automatically (or go to http://192.168.4.1)
4. **Choose your mode:**

### Generic MQTT Mode Setup

```
Step 1: WiFi Configuration
  └─ Enter your WiFi credentials

Step 2: MQTT Broker
  ├─ Host: mqtt.example.com
  ├─ Port: 1883 (or 8883 for SSL)
  ├─ Username & Password
  └─ Topic prefix: opendtu / ahoydtu / custom

Step 3: Inverter Discovery
  └─ Scans for Hoymiles inverters automatically

✅ Setup complete! Data publishing to your MQTT broker.
```

### mypvlog Direct Mode Setup

```
Step 1: WiFi Configuration
  └─ Enter your WiFi credentials

Step 2: MyPVLog.net Login
  ├─ Sign in with Google (or email/password)
  └─ Don't have an account? Sign up free!

Step 3: Automatic Provisioning
  ├─ ✓ DTU registered in your account
  ├─ ✓ MQTT credentials configured
  ├─ ✓ Inverters discovered
  └─ ✓ Dashboard ready!

✅ Setup complete! View your dashboard at mypvlog.net
```

---

## 🌍 Supported Inverters

### Currently Supported

| Brand | Models | Protocol | Status |
|-------|--------|----------|--------|
| **Hoymiles** | HM-300 to HM-1500 | NRF24L01+ | ✅ Full support |
| **Hoymiles** | HMS-800 to HMS-2000 | CMT2300A | ✅ Full support |
| **Hoymiles** | HMT-1600 to HMT-2250 | CMT2300A | ✅ Full support |
| **TSUN** | TSOL-M350, M800, M1600 (serial starting with "11") | NRF24L01+ | ✅ Full support |
| **APSystems** | YC600, YC1000, QT2 | ECU Gateway (TCP) | ✅ Full support |
| **APSystems** | QS1, DS3 | ECU Gateway (TCP) | ✅ Full support |

**Note:** TSUN inverters use the same protocol as Hoymiles HM series. APSystems requires an ECU-B/ECU-R/ECU-C gateway device on your local network.

### Planned Support (Roadmap)

| Brand | Models | Protocol | ETA |
|-------|--------|----------|-----|
| **SolarEdge** | All inverters | Modbus RTU/TCP | Q3 2026 |
| **Enphase** | IQ7, IQ8 | Local API | Q3 2026 |
| **Deye/Sunsynk** | All hybrid | Modbus RTU | Q4 2026 |

---

## 🎯 Feature Comparison

| Feature | Generic MQTT | mypvlog Direct |
|---------|--------------|----------------|
| Setup time | 5-10 minutes | **2 minutes** |
| MQTT broker | User provides | Auto-configured |
| Poll interval | 5-15s (configurable) | **2-5s (optimized)** |
| Historical data | User's storage | InfluxDB (subscription) |
| Cloud dashboard | User's platform | ✅ mypvlog.net |
| Mobile app | User's app | ✅ mypvlog.net app |
| Power limiting | Manual MQTT | ✅ Auto (Plus/Pro tiers) |
| OTA updates | Manual upload | ✅ Auto-push |
| API access | User's MQTT | ✅ REST API (Pro tier) |
| Cost | Free (self-hosted) | Free tier + subscriptions |

### MyPVLog.net Subscription Tiers

- **Free:** 1 DTU, 1 inverter, 7 days history
- **Basic (€1/mo):** Unlimited history, email alerts
- **Plus (€3.99/mo):** 2 DTUs, analytics, power limiting
- **Pro (€7.99/mo):** Unlimited DTUs, API access, priority support

---

## 📦 Firmware Variants

We provide pre-built firmware for different hardware configurations:

| Variant | Platform | Radio/Comm | Size | Use Case |
|---------|----------|------------|------|----------|
| `esp32-nrf24` | ESP32 | NRF24L01+ | 1.2MB | **Hoymiles HM / TSUN** ⭐ Most common |
| `esp32-dual` | ESP32 | NRF24 + CMT2300A | 1.4MB | **HM + HMS/HMT** ⭐ Recommended |
| `esp32s3-dual` | ESP32-S3 | NRF24 + CMT2300A | 1.5MB | Latest hardware, fastest |
| `esp8266-nrf24` | ESP8266 | NRF24L01+ | 900KB | Budget option (limited features) |
| `esp32-apsystems` | ESP32 | WiFi/TCP | 1.1MB | **APSystems ECU Gateway** |

**Notes:**
- **Hoymiles HM & TSUN**: Use `esp32-nrf24` or `esp32-dual` variants
- **Hoymiles HMS/HMT**: Use `esp32-dual` or `esp32s3-dual` variants
- **APSystems**: Use `esp32-apsystems` variant (requires ECU-R/ECU-C on LAN)

Download from [Releases](https://github.com/mypvlog/firmware/releases/latest).

---

## 🔧 Development

### Requirements

- [PlatformIO Core](https://platformio.org/) or [PlatformIO IDE](https://platformio.org/platformio-ide)
- ESP32 or ESP8266 development board
- nRF24L01+ module (and/or CMT2300A for HMS/HMT)

### Build from Source

```bash
# Clone repository
git clone https://github.com/mypvlog/firmware.git
cd firmware

# Install dependencies (automatic with PlatformIO)
pio lib install

# Build all variants
pio run

# Build specific variant
pio run -e esp32-nrf24

# Upload to device
pio run -e esp32-nrf24 -t upload

# Monitor serial output
pio device monitor -b 115200
```

### Project Structure

```
firmware/
├── src/                    # Main application code
│   ├── main.cpp           # Entry point
│   ├── wifi_manager.*     # WiFi & captive portal
│   ├── mqtt_generic.*     # Generic MQTT mode
│   ├── mqtt_mypvlog.*     # mypvlog Direct mode
│   ├── web_server.*       # Local web UI
│   ├── hoymiles_hm.*      # Hoymiles HM / TSUN protocol (NRF24)
│   ├── hoymiles_hms.*     # Hoymiles HMS/HMT protocol (CMT2300A)
│   ├── hoymiles_protocol.h # Shared Hoymiles protocol utilities
│   └── apsystems_ecu.*    # APSystems ECU client (TCP)
├── lib/                   # Custom libraries
│   ├── RF24/              # NRF24 driver
│   └── CMT2300A/          # CMT2300A driver
├── data/                  # Web UI (HTML/CSS/JS)
├── platformio.ini         # Build configuration
└── .github/workflows/     # CI/CD
```

### Testing

```bash
# Run unit tests
pio test

# Run on device
pio test -e esp32-nrf24
```

---

## 🤝 Contributing

We welcome contributions! Here's how you can help:

### Reporting Bugs

1. Check [existing issues](https://github.com/mypvlog/firmware/issues)
2. Open a [new issue](https://github.com/mypvlog/firmware/issues/new) with:
   - Firmware version
   - Hardware (ESP32/ESP8266, radio module)
   - Steps to reproduce
   - Serial output logs

### Feature Requests

Open an issue with the `enhancement` label and describe:
- Your use case
- Expected behavior
- Why it benefits other users

### Pull Requests

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Make your changes
4. Test thoroughly
5. Commit with clear messages
6. Push to your fork
7. Open a Pull Request

**Code Style:**
- Follow existing code formatting
- Comment non-obvious logic
- Add unit tests for new features

---

## 📜 License

This project is licensed under the **MIT License** - see the [LICENSE](LICENSE) file for details.

**TL;DR:** You can use, modify, and distribute this firmware freely, even commercially. Just include the original license.

---

## 🙏 Acknowledgments

- **OpenDTU Team** - Reverse-engineered Hoymiles protocol
- **AhoyDTU Team** - Alternative Hoymiles implementation
- **RF24 Library** - NRF24L01+ driver
- **PlatformIO** - Cross-platform build system
- **Espressif** - ESP32/ESP8266 SDK

---

## 🔗 Links

- **Documentation:** [docs.mypvlog.net](https://docs.mypvlog.net) (coming soon)
- **Web Flasher:** [flash.mypvlog.net](https://flash.mypvlog.net) (coming soon)
- **Dashboard:** [mypvlog.net](https://mypvlog.net)
- **Issues:** [GitHub Issues](https://github.com/mypvlog/firmware/issues)
- **Discussions:** [GitHub Discussions](https://github.com/mypvlog/firmware/discussions)

---

## ❓ FAQ

**Q: Is this compatible with OpenDTU?**
A: Yes! In Generic MQTT mode, we use the same topic structure and protocol as OpenDTU.

**Q: Can I switch between modes later?**
A: Yes, you can change modes anytime from the web UI settings.

**Q: Do I need a mypvlog.net account?**
A: Only for mypvlog Direct mode. Generic MQTT mode works standalone.

**Q: Does this work offline?**
A: Generic MQTT mode works completely offline. mypvlog Direct mode requires internet for cloud features.

**Q: Is my data secure?**
A: mypvlog Direct mode uses SSL/TLS encryption. All credentials are stored encrypted on the device.

**Q: Can I contribute to other inverter brands?**
A: Absolutely! Check our [Contributing Guide](CONTRIBUTING.md) and open an issue to discuss.

---

## 📊 Status

- ✅ **Generic MQTT Mode:** Production ready
- 🚧 **mypvlog Direct Mode:** In development (beta Q1 2026)
- 🚧 **Web Flasher:** In development
- 📅 **Multi-brand support:** Planned (Q2-Q4 2026)

---

Made with ☀️ by the MyPVLog.net team and open source contributors.

**Star ⭐ this repository if you find it useful!**
