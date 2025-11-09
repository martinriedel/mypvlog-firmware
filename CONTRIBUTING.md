# Contributing to MyPVLog Firmware

Thank you for considering contributing to MyPVLog Firmware! We welcome contributions from the community.

## Code of Conduct

By participating in this project, you agree to maintain a respectful and inclusive environment for everyone.

## How Can I Contribute?

### Reporting Bugs

Before creating a bug report, please check if the issue has already been reported. If it has, add a comment to the existing issue instead of opening a new one.

**Great bug reports include:**
- A clear, descriptive title
- Exact steps to reproduce
- Expected vs. actual behavior
- Firmware version and hardware details
- Serial output logs (if applicable)
- Screenshots (if applicable)

### Suggesting Enhancements

Enhancement suggestions are tracked as GitHub issues. When creating an enhancement suggestion, include:
- A clear use case
- Expected behavior
- Why this benefits other users
- Any alternative solutions you've considered

### Adding Support for New Inverter Brands

We're actively looking for contributors to add support for other inverter manufacturers!

**Steps:**
1. Open an issue describing the inverter brand/model
2. Research the communication protocol (Modbus, local API, etc.)
3. Create a fork and implement the protocol
4. Test with real hardware
5. Submit a pull request

**Inverter brands we'd love support for:**
- APsystems (EZ1, QS1)
- SolarEdge (all models)
- Enphase (IQ7, IQ8)
- Deye/Sunsynk (hybrid inverters)
- Others!

### Pull Requests

1. **Fork the repository**
   ```bash
   git clone https://github.com/mypvlog/firmware.git
   cd firmware
   git checkout -b feature/amazing-feature
   ```

2. **Make your changes**
   - Follow the existing code style
   - Add comments for complex logic
   - Update documentation if needed

3. **Test thoroughly**
   ```bash
   # Build for all variants
   pio run

   # Run tests
   pio test
   ```

4. **Commit with clear messages**
   ```bash
   git commit -m "feat: Add support for APsystems EZ1"
   ```

   Use conventional commit prefixes:
   - `feat:` - New feature
   - `fix:` - Bug fix
   - `docs:` - Documentation changes
   - `refactor:` - Code refactoring
   - `test:` - Adding tests
   - `chore:` - Maintenance tasks

5. **Push and create a Pull Request**
   ```bash
   git push origin feature/amazing-feature
   ```

   In your PR description, include:
   - What changes were made
   - Why they were made
   - How they were tested
   - Related issues (if any)

## Development Setup

### Prerequisites

- [PlatformIO](https://platformio.org/) (CLI or IDE)
- ESP32 or ESP8266 development board
- nRF24L01+ module (for testing Hoymiles HM)

### First-Time Setup

```bash
# Clone your fork
git clone https://github.com/YOUR_USERNAME/firmware.git
cd firmware

# Install dependencies (automatic with PlatformIO)
pio lib install

# Build
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
│   ├── config.h           # Configuration
│   ├── wifi_manager.*     # WiFi management
│   ├── web_server.*       # Web UI backend
│   ├── mqtt_generic.*     # Generic MQTT mode
│   ├── mqtt_mypvlog.*     # MyPVLog Direct mode
│   ├── hoymiles_hm.*      # Hoymiles HM protocol
│   └── hoymiles_hms.*     # Hoymiles HMS/HMT protocol
├── lib/                   # Custom libraries
├── data/                  # Web UI files (HTML/CSS/JS)
├── test/                  # Unit tests
└── platformio.ini         # Build configuration
```

## Code Style

### C++ Style

```cpp
// Use camelCase for functions and variables
void pollInverters() {
    uint8_t inverterCount = getInverterCount();
}

// Use PascalCase for classes
class HoymilesHM {
public:
    void begin();
private:
    uint8_t m_inverterCount;  // Member variables prefixed with m_
};

// Constants in UPPER_CASE
#define MAX_INVERTERS 8
const uint16_t POLL_INTERVAL = 5000;
```

### Comments

```cpp
/**
 * Poll all registered Hoymiles HM inverters
 *
 * Sends requests to each inverter and waits for response.
 * Non-blocking operation using millis() timing.
 *
 * @return Number of successful responses
 */
uint8_t pollInverters() {
    // Implementation here
}
```

## Testing

### Unit Tests

Place tests in the `test/` directory:

```cpp
// test/test_hoymiles.cpp
#include <unity.h>
#include "hoymiles_hm.h"

void test_inverter_add() {
    HoymilesHM hm;
    bool result = hm.addInverter(116181672324);
    TEST_ASSERT_TRUE(result);
}

void setup() {
    UNITY_BEGIN();
    RUN_TEST(test_inverter_add);
    UNITY_END();
}

void loop() {}
```

Run tests:
```bash
pio test
```

### Hardware Testing

Before submitting a PR:
- [ ] Test on real ESP32 hardware
- [ ] Test WiFi connection/reconnection
- [ ] Test MQTT publishing
- [ ] Test with real inverter (if applicable)
- [ ] Verify OTA updates work
- [ ] Check memory usage

## Documentation

When adding new features, update:
- `README.md` - User-facing documentation
- Code comments - For maintainers
- `CHANGELOG.md` - List your changes

## License

By contributing, you agree that your contributions will be licensed under the MIT License.

## Questions?

- 💬 Open a [Discussion](https://github.com/mypvlog/firmware/discussions)
- 🐛 Report [Issues](https://github.com/mypvlog/firmware/issues)
- 📧 Email: support@mypvlog.net

## Recognition

Contributors will be recognized in:
- `CONTRIBUTORS.md` file
- Release notes for significant contributions
- GitHub contributors page

Thank you for making MyPVLog Firmware better! ☀️
