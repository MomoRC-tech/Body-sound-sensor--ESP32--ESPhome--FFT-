# MPU FFT JSON External Component

Custom ESPHome component for ESP32-based vibration analysis using MPU6050 accelerometer and real-time FFT processing.

## Structure

```
custom_components/mpu_fft_json/
├── __init__.py          # Component registration and I2C setup
├── sensor.py            # Sensor platform (RMS, CPU load, diagnostics)
├── text_sensor.py       # Text sensor platform (spectrum JSON)
└── mpu_fft_json.h       # C++ implementation using ESP-DSP
```

## Features

- **On-device FFT**: FFT with configurable sample rate and size (default 1 kHz / 512)
- **Multi-band spectrum**: Configurable band count (default 16), published as JSON
- **RMS calculation**: Root mean square acceleration in g units
- **CPU load monitoring**: Real-time processing overhead percentage
- **50% window overlap**: Hamming windowing for smooth spectral analysis
 - **Diagnostic sensors**: Bin size, sample rate, FFT size, band count, max analysis Hz

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
  # Optional tuning
  sample_frequency: 1000.0
  fft_samples: 512
  fft_bands: 16
  window_shift: 0            # 0 = 50% overlap
  max_analysis_hz: 300.0     # Cap analyzed band range
  dc_alpha: 0.01             # High-pass coefficient
  load_window_us: 1000000    # CPU load window

sensor:
  - platform: mpu_fft_json
    mpu_fft_json_id: mpu_fft
    rms:
      name: "Vibration RMS"
    cpu_load:
      name: "FFT CPU Load"
    # Optional diagnostics for HA device page
    bin_hz:
      name: "FFT Bin Size"
      entity_category: diagnostic
    sample_frequency:
      name: "Sample Rate"
      unit_of_measurement: "Hz"
      entity_category: diagnostic
    fft_samples:
      name: "FFT Size"
      entity_category: diagnostic
    fft_bands:
      name: "Band Count"
      entity_category: diagnostic
    max_analysis_hz:
      name: "Max Analysis Hz"
      unit_of_measurement: "Hz"
      entity_category: diagnostic

text_sensor:
  - platform: mpu_fft_json
    mpu_fft_json_id: mpu_fft
    spectrum:
      name: "Spectrum JSON"
```

## Dependencies

- **ESPHome**: 2025.10.5+
- **ESP-DSP**: Header `esp_dsp.h` must be available (bundled with ESP-IDF/Arduino-ESP32 cores)
- **I2C**: Required for MPU6050 communication

## Technical Details

### DSP Parameters (defaults; configurable via YAML)
- Sample rate: 1000 Hz
- FFT size: 512 samples
- Window: Hamming
- Overlap: 50% (256 samples or `window_shift: 0`)
- Frequency bands: 16 (0–fs/2)
- Max analysis Hz: 300.0 (clamped to Nyquist)
- DC removal: High-pass filter (α=0.01)

### Output Format (JSON)
```json
{
  "fs": 1000.0,
  "n": 512,
  "bin_hz": 1.953,
  "rms": 0.123456,
  "peak_hz": 42.15,
  "max_analysis_hz": 300.0,
  "bands": [0.12, 0.34, 0.56, ...],
  "band_center": [9.4, 28.1, ...],
  "band_low":    [0.0, 18.8, ...],
  "band_high":   [18.8, 37.5, ...]
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
