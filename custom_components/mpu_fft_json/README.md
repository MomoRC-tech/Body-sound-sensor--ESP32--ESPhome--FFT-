# MPU FFT JSON External Component

Custom ESPHome component for ESP32-based vibration analysis using MPU6050 accelerometer and real-time FFT processing.

## Structure

```
custom_components/mpu_fft_json/
├── __init__.py          # Component registration and I2C setup
├── sensor.py            # Sensor platform (RMS, CPU load)
├── text_sensor.py       # Text sensor platform (spectrum JSON)
└── mpu_fft_json.h       # C++ implementation with arduinoFFT v2
```

## Features

- **On-device FFT**: 512-sample FFT at 1 kHz using arduinoFFT v2.0.4
- **Multi-band spectrum**: 16 frequency bands published as JSON
- **RMS calculation**: Root mean square acceleration in g units
- **CPU load monitoring**: Real-time processing overhead percentage
- **50% window overlap**: Hamming windowing for smooth spectral analysis

## Configuration

```yaml
external_components:
  - source:
      type: local
      path: custom_components

i2c:
  sda: 21
  scl: 22
  frequency: 400kHz

mpu_fft_json:
  id: mpu_fft
  address: 0x68  # MPU6050 I2C address (0x68 or 0x69)

sensor:
  - platform: mpu_fft_json
    mpu_fft_json_id: mpu_fft
    rms:
      name: "Vibration RMS"
    cpu_load:
      name: "FFT CPU Load"

text_sensor:
  - platform: mpu_fft_json
    mpu_fft_json_id: mpu_fft
    spectrum:
      name: "Spectrum JSON"
```

## Dependencies

- **ESPHome**: 2025.10.5+
- **arduinoFFT**: ^2.0.4 (automatically installed)
- **I2C**: Required for MPU6050 communication

## Technical Details

### DSP Parameters
- Sample rate: 1000 Hz
- FFT size: 512 samples
- Window: Hamming
- Overlap: 50% (256 samples)
- Frequency bands: 16 (0–500 Hz)
- DC removal: High-pass filter (α=0.01)

### Output Format (JSON)
```json
{
  "fs": 1000.0,
  "n": 512,
  "bin_hz": 1.953,
  "rms": 0.123456,
  "peak_hz": 42.15,
  "bands": [0.12, 0.34, 0.56, ...]
}
```

## Migration from Custom Platform

This component replaces the legacy `platform: custom` approach (removed in ESPHome 2025.10.5) with a proper external component using ESPHome's code generation framework.

### Before (deprecated)
```yaml
sensor:
  - platform: custom
    lambda: |-
      auto mpu_fft = new MPUFftJsonComponent();
      ...
```

### After (current)
```yaml
sensor:
  - platform: mpu_fft_json
    mpu_fft_json_id: mpu_fft
    rms:
      name: "Vibration RMS"
```

## License

See project root LICENSE file.
