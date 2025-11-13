## AI Coding Agent Instructions (Concise Guide)

Purpose: ESP32 + MPU60x0/65/92 IMU sampled via ESPHome external component; performs on‑device FFT, aggregates band energies, publishes numeric sensors + JSON spectrum to Home Assistant.

### Architecture & Data Flow
ESPHome YAML -> Python codegen (`custom_components/mpu_fft_json/*.py`) sets up C++ runtime (`mpu_fft_json.h`).
Loop: I²C read -> HPF (DC removal) -> fill window -> 512‑pt (configurable) Hamming FFT (50% overlap default) -> band aggregation (1..64) up to `max_analysis_hz` (default 300 Hz) -> JSON publish.
JSON + numeric sensors exposed via ESPHome standard sensor/text_sensor APIs; minimal heap (arrays allocated once in setup).

### Key Files
`body_sound_sensor.yaml` reference config & filters.
`custom_components/mpu_fft_json/__init__.py` schema + library dep (`arduinoFFT ^2.0.4`).
`sensor.py` / `text_sensor.py` bind sensors via `set_*` methods.
`mpu_fft_json.h` implements sampling, FFT, band energy, timing metadata, CPU load.
`examples/python/analyze_spectrum.py` downstream parsing & feature extraction.

### Core Configuration Pattern (YAML)
```yaml
external_components:
  - source: {type: local, path: custom_components}
mpu_fft_json:
  id: mpu_fft
  address: 0x68          # use 0x69 if needed
  sample_frequency: 1000.0  # 10..5000
  fft_samples: 512          # 128..2048 (auto power-of-two clamp)
  fft_bands: 16             # 1..64
  window_shift: 0           # 0 => auto 50% overlap
  max_analysis_hz: 300.0    # 0 => Nyquist
  dc_alpha: 0.01
  load_window_us: 1000000
```
Add sensors:
```yaml
sensor:
  - platform: mpu_fft_json
    mpu_fft_json_id: mpu_fft
    rms: { name: "RMS" }
    cpu_load: { name: "CPU Load" }
text_sensor:
  - platform: mpu_fft_json
    mpu_fft_json_id: mpu_fft
    spectrum: { name: "Spectrum JSON" }
```

### JSON Contract (Stable Fields)
`fs, n, bin_hz, rms, peak_hz, max_analysis_hz, ts_ms, win_ms, hop_ms, seq, epoch_ms (if time sync), bands[], band_center[], band_low[], band_high[]`.
Backward compatible: only add new fields; bump version if removing/renaming.

### Extension Patterns
Add numeric sensor: declare optional key in `sensor.py`, create `new_sensor`, add corresponding `set_*_sensor` in header and publish in `process_window_()`.
Add JSON field: modify assembly in `mpu_fft_json.h` near JSON construction block; keep ordering & existing names.
Performance tuning: reduce `sample_frequency`, `fft_samples`, or `fft_bands`; increase `window_shift` (> n/2 lowers update rate & CPU load). Watch `cpu_load` (< ~75%).

### Timing Semantics
`ts_ms` monotonic (boot-relative) at window center; `seq` increments each window; `epoch_ms` derived once ESPHome time becomes valid (base offset stored). Use `seq` + `ts_ms` to detect gaps.

### Development Workflow (PowerShell)
```powershell
esphome config body_sound_sensor.yaml
esphome compile body_sound_sensor.yaml
esphome upload body_sound_sensor.yaml
esphome logs body_sound_sensor.yaml
./scripts/setup.ps1        # local env prep
./scripts/release.ps1 -VersionBump patch  # releases
```

### Common Checks
Logs: confirm MPU wake, `Configured fs=...` line, periodic JSON text updates.
Mounting: rigid mount improves SNR; soft foam reduces spectral clarity.
I²C issues: verify address 0x68 vs 0x69; inspect logs for wake failure.

### Adding Time Sync
Include ESPHome `time:` block; once valid, `epoch_ms` appears automatically (no extra handling needed).

### Downstream Analysis
Use `examples/python/analyze_spectrum.py` after `pip install -r examples/python/requirements.txt`; treat `bands[]` as feature vector for classification/anomaly detection.

### Guardrails
Do not commit `secrets.yaml` or build artifacts. Preserve JSON field names. Keep window overlap logic (shift normalization) unchanged unless adding explicit configuration.

Feedback: Request clarification before changing public JSON contract or release workflow.

