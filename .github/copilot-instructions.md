# AI Agent Guide: ESP32 Vibration FFT Analyzer

This repo builds an ESPHome external component that samples an MPU60x0 IMU on ESP32, computes an on-device FFT, aggregates band energies, and publishes both numeric sensors and a compact JSON spectrum to Home Assistant.

## Big Picture
- Architecture: `ESPHome YAML` → external component `custom_components/mpu_fft_json` (Python codegen + C++ `mpu_fft_json.h`) → publishes sensors (`RMS`, `CPU load`, diagnostics) and a text sensor (`Spectrum JSON`).
- Data flow: I²C MPU → ESP32 samples at $f_s$ (default 1000 Hz) → 512-point FFT with Hamming window and 50% overlap → band energies computed up to `max_analysis_hz` → JSON payload with spectrum metadata and bands.
- Why this structure: replaces removed `platform: custom` with a proper ESPHome external component; JSON output enables lightweight downstream ML/analytics while keeping on-device compute small.

## Key Files
- `body_sound_sensor.yaml`: Reference device config; shows all available sensors, filters, and defaults.
- `custom_components/mpu_fft_json/__init__.py`: Component schema, validation, and library dependency (`arduinoFFT ^2.0.4`).
- `custom_components/mpu_fft_json/sensor.py` and `text_sensor.py`: Register numeric sensors and JSON text sensor via codegen setters.
- `custom_components/mpu_fft_json/mpu_fft_json.h`: C++ implementation: sampling, HPF DC removal, FFT, band aggregation, JSON assembly, CPU load.
- `examples/python/analyze_spectrum.py`: Downstream parsing, visualization, simple rules, and feature extraction.
- `scripts/setup.ps1`, `scripts/release.ps1`: Local setup, validation, and release helper workflows.

## How To Run + Debug
- Validate/build/upload/logs (ESPHome CLI):
  - `esphome config body_sound_sensor.yaml`
  - `esphome compile body_sound_sensor.yaml`
  - `esphome upload body_sound_sensor.yaml`
  - `esphome logs body_sound_sensor.yaml`
- Quick setup (Windows/PowerShell): `./scripts/setup.ps1`
- Monitor common states in logs: I²C detect at `0x68/0x69`, MPU wake-up, `fs`, `n`, `bands`, and periodic JSON text updates.
- Enable more verbosity: set `logger.level: DEBUG` in YAML.

## Configuration Patterns (ESPHome)
- Always include the external component locally:
  ```yaml
  external_components:
    - source:
        type: local
        path: custom_components
  ```
- Component block (tunable at compile time):
  ```yaml
  mpu_fft_json:
    id: mpu_fft
    address: 0x68
    sample_frequency: 1000.0   # Hz; clamped 10..5000
    fft_samples: 512           # power of two; clamped 64..4096
    fft_bands: 16              # 1..64
    window_shift: 0            # 0 => 50% overlap (auto set to n/2)
    max_analysis_hz: 300.0     # 0/omit => use Nyquist
    dc_alpha: 0.01             # HPF coefficient
    load_window_us: 1000000    # CPU load averaging window
  ```
- Exposed entities (example):
  ```yaml
  sensor:
    - platform: mpu_fft_json
      mpu_fft_json_id: mpu_fft
      rms: { name: "Body Sound Sensor RMS" }
      cpu_load: { name: "Body Sound FFT CPU Load" }
      # Diagnostics (optional): bin_hz, sample_frequency, fft_samples, fft_bands, max_analysis_hz

  text_sensor:
    - platform: mpu_fft_json
      mpu_fft_json_id: mpu_fft
      spectrum: { name: "Body Sound Sensor Spectrum JSON" }
  ```
- Filters: prefer ESPHome `sliding_window_moving_average` for RMS/CPU smoothing (see `body_sound_sensor.yaml`).

## JSON Output Contract (device → HA)
- Fields: `fs`, `n`, `bin_hz`, `rms`, `peak_hz`, `max_analysis_hz`, `bands[]`, `band_center[]`, `band_low[]`, `band_high[]`.
- Timing metadata: `ts_ms` (monotonic ms since boot at window center), `win_ms` (window length), `hop_ms` (hop size), `seq` (incrementing window index).
 - Timing metadata: `ts_ms` (monotonic ms since boot at window center), `win_ms` (window length), `hop_ms` (hop size), `seq` (incrementing window index), `epoch_ms` (Unix ms at window center when `time:` is configured and synced).
- Semantics: `$\text{bin\_hz} = f_s / n$`, $f_{Nyquist}=f_s/2$, `max_analysis_hz` clamps band top; band width is `f_top/fft_bands`.
- Stability: additional fields are backward compatible; treat the set as a public contract—if you rename/remove, bump minor/major and update docs/tests.

## Conventions & Coding Patterns
- External component uses ESPHome codegen: Python defines schema + setters; C++ holds logic and publishes via pointers set by codegen.
- Add a new numeric sensor: add setter in `mpu_fft_json.h`, wire publish in processing path, declare option in `sensor.py` and call `parent.set_*_sensor` in `to_code`.
- Add a new text sensor or JSON field: extend C++ JSON assembly; expose via `text_sensor.py` if adding a separate entity.
- Units: RMS in `g`, CPU in `%`, `bin_hz` in `Hz/bin`, sample rate in `Hz`.
- Performance guardrails: clamp `sample_frequency`, `fft_samples`, and bands to sane ranges; component already normalizes power-of-two and overlap.

## CI/CD & Releases
- CI validates: ESPHome YAML, compiles firmware for verification, C++ style/syntax checks, Python example tests.
- Release flow (Windows):
  - `./scripts/release.ps1 -VersionBump [patch|minor|major] [-ReleaseNotes "..."]`
  - Confirms version, runs local checks, pushes, and triggers GitHub Actions release.
- Policy: CI-built firmware is for verification only; users build/flash via ESPHome Dashboard or CLI. Do not commit build artifacts or `secrets.yaml`.

## Analysis & ML (examples)
- Parse/visualize/classify spectrum: `examples/python/analyze_spectrum.py`.
- Install deps: in `examples/python`: `pip install -r requirements.txt`; run the script to inspect `bands[]`, plot linear/dB, and extract features.

## Gotchas
- I²C address defaults to `0x68`; some boards use `0x69` (set in YAML via `address:` on the component; CI/codegen schema defaults to `0x68`).
- High CPU load (> ~70–80%): reduce `sample_frequency`, `fft_samples`, or `fft_bands`, or raise `window_shift` from 50% overlap.
- Mounting matters: rigid mounting dramatically improves SNR; foam/soft adhesives dampen signal.

## Quick Commands (PowerShell)
```powershell
# Validate / compile / upload / logs
esphome config body_sound_sensor.yaml
esphome compile body_sound_sensor.yaml
esphome upload body_sound_sensor.yaml
esphome logs body_sound_sensor.yaml

# Setup helper
./scripts/setup.ps1

# Release helper
./scripts/release.ps1 -VersionBump patch
```
