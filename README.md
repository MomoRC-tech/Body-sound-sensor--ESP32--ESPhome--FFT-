# ESP32 Vibration FFT Analyzer

<!-- VERSION:START -->
Latest Release: v0.9.1 (2025-11-13)
Tested with ESPHome: Version: 2025.10.5
<!-- VERSION:END -->

[![ESPHome CI/CD](https://github.com/MomoRC-tech/Body-sound-sensor--ESP32--ESPhome--FFT-/actions/workflows/esphome-ci.yml/badge.svg)](https://github.com/MomoRC-tech/Body-sound-sensor--ESP32--ESPhome--FFT-/actions/workflows/esphome-ci.yml)
CI status is reflected by the badge above; the latest run is passing.

An ESP32-based vibration sensor node that measures **structure-borne noise (Körperschall)** from household devices like well pumps, ventilation systems, and motors. It performs real-time FFT analysis on-board and publishes spectral data to Home Assistant via ESPHome.

## Overview

This project turns an ESP32 and an IMU into a smart **body-sound / vibration sensor**:

- Samples acceleration at 1 kHz
- Runs a 512-point FFT on-device
- Aggregates the spectrum into configurable bands
- Sends compact JSON spectra and RMS values to Home Assistant
- Monitors CPU load so you can see how hard the ESP32 is working

Use it to detect pump/fan activity, identify devices by their vibration signature, or feed data into ML pipelines running on your Raspberry Pi / Home Assistant ecosystem.

## Features

- **Real-time FFT Analysis**: 512-sample FFT @ 1000 Hz on ESP32
- **Multi-band Spectral Energies**: 16 frequency bands for device fingerprinting
- **Home Assistant Integration**: Seamless integration via ESPHome
- **JSON Spectrum Output**: Compact spectral data for ML/analysis
- **CPU Load Monitoring**: Track ESP32 performance in real-time
- **High-Pass Filtering**: Removes DC component (gravity)
- **50% Overlap**: Smooth spectral updates ~4/second

## Quick Start

1. **Prepare Hardware**
   - ESP32 D1 mini-style board (Wemos D1 mini32 or similar)
   - MPU6050 / MPU6500 / MPU9250 IMU
   - Rigid mounting to the vibrating structure (pipe, wall, housing)

2. **Clone / Copy Project**
   - Place this repository (or at least `body_sound_sensor.yaml` and `custom_components/`) in your ESPHome directory, e.g. `config/esphome/` in Home Assistant.

3. **Configure Secrets**
   - Copy `secrets.yaml.example` → `secrets.yaml` and edit:
     - `wifi_ssid`, `wifi_password`
     - `api_encryption_key` (32-byte base64)
     - `ota_password`

4. **Build & Upload**
   - Via ESPHome Dashboard, build and upload `body_sound_sensor.yaml`, or use CLI:
     ```bash
     esphome upload body_sound_sensor.yaml
     ```

5. **Integrate with Home Assistant**
   - The device will appear via the ESPHome integration.
   - Use the exposed sensors (`RMS`, `CPU load`, `Spectrum JSON`) in automations, dashboards, or external analysis.

## Hardware

Basic hardware requirements are:

- **MCU**: ESP32 D1 mini-style board (ESP32-WROOM)
- **Sensor**: MPU-9250 / MPU-6500 / MPU-6050 IMU
- **Connection**: I²C bus (3.3 V)

<details>
<summary><strong>Expand for detailed hardware configuration & wiring</strong></summary>

### Your Configuration

- **MCU**: ESP32-WROOM D1 Mini
- **Sensor**: MPU6050 3-axis accelerometer
- **I²C Address**: 0x68 (default)
- **WiFi**: MOMOWLAN

### Components

- **MCU**: ESP32 D1 mini-style board (Wemos D1 mini32 or similar)
- **Sensor**: MPU-9250 / MPU-6500 / MPU-6050 IMU (3-axis accelerometer)
- **Connection**: I²C bus

### Wiring

| MPU Pin | ESP32 Pin | Description   |
|---------|-----------|---------------|
| VCC     | 3.3V      | Power supply  |
| GND     | GND       | Ground        |
| SDA     | GPIO 21   | I²C Data      |
| SCL     | GPIO 22   | I²C Clock     |

**I²C Address**: Typically `0x68` (or `0x69` depending on AD0 pin configuration)

### Mounting Recommendations

⚠️ **Critical for accurate measurements:**

- Mount sensor **rigidly** to the structure carrying the vibration
- Use screws + bracket OR rigid mounting glue/high-strength tape
- Mount close to vibration source (e.g., near pump pipe or supporting wall)
- **Avoid**: Thick, soft, foam-like adhesives (they dampen vibrations)

</details>

## Exposed Sensors & Data

### 1. Vibration RMS (`sensor`)

- **Name**: `Body Sound Vibration RMS`
- **Unit**: `g` (gravitational acceleration)
- **Purpose**: Overall vibration level for on/off detection and thresholds

### 2. CPU Load (`sensor`)

- **Name**: `Body Sound FFT CPU Load`
- **Unit**: `%`
- **Purpose**: Monitor ESP32 performance (should stay < 70–80%)

### 3. Spectrum JSON (`text_sensor`)

- **Name**: `Body Sound Spectrum JSON`
- **Format**: JSON object with spectral data

<details>
<summary><strong>Expand for JSON example and field description</strong></summary>

**Example JSON Output:**
```json
{
  "fs": 1000.0,
  "n": 512,
  "bin_hz": 1.953,
  "rms": 0.012345,
  "peak_hz": 49.2,
  "bands": [12.5, 8.3, 15.7, 22.1, 18.9, 11.2, 9.4, 7.8, 6.5, 5.2, 4.1, 3.3, 2.8, 2.1, 1.5, 1.2]
}
```

**JSON Fields:**
- `fs`: Sample frequency (Hz)
- `n`: FFT size (samples)
- `bin_hz`: Frequency resolution (Hz/bin)
- `rms`: Time-domain RMS vibration (g)
- `peak_hz`: Dominant frequency (Hz)
- `bands`: Array of 16 band energies (arbitrary units)

</details>

## Configuration & Tuning

The following parameters can be tuned in `mpu_fft_json.h`:

| Parameter           | Default   | Description                                  |
|---------------------|-----------|----------------------------------------------|
| `SAMPLE_FREQUENCY`  | 1000 Hz   | Sampling rate (Nyquist = 500 Hz)            |
| `FFT_SAMPLES`       | 512       | FFT window size (power of 2)                |
| `FFT_BANDS`         | 16        | Number of frequency bands                   |
| `WINDOW_SHIFT`      | 256       | Overlap amount (50%)                        |
| `DC_ALPHA`          | 0.01      | High-pass filter coefficient                |
| `LOAD_WINDOW_US`    | 1,000,000 | CPU load averaging window (µs)              |

**Performance Considerations:**

- Lower `SAMPLE_FREQUENCY` → Less CPU load, lower Nyquist frequency
- Smaller `FFT_SAMPLES` (256) → Faster FFT, coarser frequency resolution
- Larger `FFT_SAMPLES` (1024) → Finer resolution, more CPU/RAM usage
- Fewer `FFT_BANDS` → Less data per frame, cheaper processing in Home Assistant / Node-RED

## Use Cases

<details>
<summary><strong>Expand for detailed use cases</strong></summary>

### 1. Device Identification

- Parse JSON `bands[]` as feature vector
- Train classifier to identify which device is running (pump, fan, etc.)
- Use different frequency signatures as "fingerprints"

### 2. On/Off Detection

- Monitor `rms` value for threshold-based detection
- Example: "Pump active" when RMS > 0.02g and bands[2–4] elevated

### 3. Condition Monitoring

- Track changes in spectral patterns over time
- Detect mechanical wear or anomalies (e.g., bearing degradation)
- Alert on significant deviations from baseline

### 4. Machine Learning Integration

- Log JSON spectra with labels to database
- Train models off-device (Raspberry Pi / PC)
- Deploy models for real-time classification or anomaly detection

</details>

## Project Structure & Documentation

<details>
<summary><strong>Expand for repository layout and documentation links</strong></summary>

### Project Structure

```text
.
├── body_sound_sensor.yaml       # ESPHome configuration
├── custom_components/
│   └── mpu_fft_json/
│       ├── sensor.py            # ESPHome codegen sensors
│       ├── text_sensor.py       # ESPHome codegen text sensors
│       └── mpu_fft_json.h       # C++ custom component (FFT processing)
├── secrets.yaml.example         # Template for WiFi/API credentials
├── scripts/
│   ├── setup.ps1                # Automated setup script
│   └── release.ps1              # Release helper script
├── .github/
│   └── workflows/
│       ├── esphome-ci.yml       # CI/CD: validate, compile, test
│       └── release.yml          # Release automation
├── docs/
│   ├── SETUP.md                 # Detailed setup guide
│   ├── HARDWARE.md              # Hardware assembly instructions
│   ├── CI_CD.md                 # CI/CD documentation
│   └── CI_CD_FAQ.md             # CI/CD FAQ and tips
├── examples/
│   └── python/                  # Python analysis scripts
└── README.md                    # This file
```

### Documentation

- Hardware: [docs/HARDWARE.md](docs/HARDWARE.md)
- Setup: [docs/SETUP.md](docs/SETUP.md)
- CI/CD: [docs/CI_CD.md](docs/CI_CD.md)
- CI/CD FAQ: [docs/CI_CD_FAQ.md](docs/CI_CD_FAQ.md)

</details>

## Troubleshooting

<details>
<summary><strong>Expand for troubleshooting tips</strong></summary>

### Sensor Not Detected

1. Check I²C wiring (SDA/SCL)
2. Verify I²C address (0x68 or 0x69)
3. Run I²C scan via ESPHome logs
4. Check sensor power supply (3.3V)

### High CPU Load (>80%)

1. Reduce `SAMPLE_FREQUENCY` (e.g., 500 Hz)
2. Decrease `FFT_SAMPLES` (e.g., 256)
3. Reduce `FFT_BANDS` (e.g., 8)
4. Increase `WINDOW_SHIFT` (less overlap)

### Noisy RMS Readings

1. Check mechanical mounting (ensure rigid)
2. Increase `DC_ALPHA` for stronger high-pass filtering
3. Add averaging filter in Home Assistant
4. Verify sensor is not picking up ESP32 self-vibration

### JSON Not Updating

1. Check ESPHome logs for errors
2. Verify `spectrum_text` sensor is registered
3. Ensure FFT processing is completing (CPU not overloaded)

</details>

## Development & CI/CD

<details>
<summary><strong>Expand for contributing and release workflow</strong></summary>

### Contributing

Contributions are welcome! Please:

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

### Releasing New Versions

To create a release, use the helper script:

```powershell
# Patch release (bug fixes)
.\scriptselease.ps1 -VersionBump patch

# Minor release (new features)
.\scriptselease.ps1 -VersionBump minor

# Major release (breaking changes)
.\scriptselease.ps1 -VersionBump major
```

Or simply ask GitHub Copilot: _"Push and release a patch version"_

See [CI/CD Documentation](docs/CI_CD.md) for details.

</details>

## License

This project is licensed under the MIT License – see the [LICENSE](LICENSE) file for details.

## Acknowledgments

- [ESPHome](https://esphome.io/) – Home automation framework
- [arduinoFFT](https://github.com/kosme/arduinoFFT) – FFT library
- Specification document contributors

## Support

- **Issues**: [GitHub Issues](https://github.com/MomoRC-tech/Body-sound-sensor--ESP32--ESPhome--FFT-/issues)
- **Discussions**: [GitHub Discussions](https://github.com/MomoRC-tech/Body-sound-sensor--ESP32--ESPhome--FFT-/discussions)

---

**Note**: This is a hobbyist project for structure-borne noise analysis. For critical industrial monitoring applications, please consult professional vibration analysis equipment and expertise.
