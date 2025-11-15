## AI Coding Agent Instructions (Concise Guide)

Purpose: ESP32 + MPU60x0/65/92 IMU using ESPHome external component; performs on‑device FFT, aggregates band energies, publishes numeric sensors + JSON spectrum to Home Assistant.

### 1. Architecture & Flow
ESPHome YAML (`body_sound_sensor.yaml`) → Python codegen (`custom_components/mpu_fft_json/*.py`) → C++ runtime (`mpu_fft_json.h`).
Processing loop: I²C read → high‑pass (DC removal) → fill window → Hamming FFT (size 128–2048, default 512) with 50% overlap (`window_shift: 0`) → band aggregation (1–64 bands limited by `max_analysis_hz`, default 300 Hz) → construct JSON + publish sensors. Heap usage kept low: buffers allocated once in `setup()`.

### 2. Key Implementation Files
`custom_components/mpu_fft_json/__init__.py` component registration (uses ESP-DSP from core; no arduinoFFT).
`sensor.py` / `text_sensor.py` define optional YAML keys → generate C++ `set_*` bindings.
`mpu_fft_json.h` sampling, HPF, FFT, band energy, timing metadata (seq, ts_ms, epoch_ms), CPU load, JSON assembly.
`examples/python/analyze_spectrum.py` downstream parsing + feature extraction (treat `bands[]` as feature vector).

### 3. Configuration Pattern (Excerpt)
```yaml
external_components:
  - source: { type: local, path: custom_components }
mpu_fft_json:
  id: mpu_fft
  address: 0x68        # 0x69 if AD0=HIGH
  sample_frequency: 1000.0
  fft_samples: 512
  fft_bands: 16
  window_shift: 0       # 0 => auto 50% overlap
  max_analysis_hz: 300.0 # 0 => Nyquist
  dc_alpha: 0.01
  load_window_us: 1000000
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

### 4. Stable JSON Contract
Fields: `fs, n, bin_hz, rms, peak_hz, max_analysis_hz, ts_ms, win_ms, hop_ms, seq, epoch_ms (after time sync), bands[], band_center[], band_low[], band_high[]`.
Contract rule: additive only (append new fields). Renaming/removing requires explicit version / user approval.

### 5. Extension Patterns
Add numeric sensor: in `sensor.py` define optional key → generate `set_<name>_sensor` in C++ → publish in `process_window_()`.
Add JSON field: edit JSON construction block in `mpu_fft_json.h` (keep ordering & existing names untouched).
Diagnostics (e.g. `bin_hz`, `fft_samples`) already supported; expose more by following existing examples in `sensor.py`.
Performance tuning: lower `sample_frequency`, `fft_samples`, or `fft_bands`; raise `window_shift` (> n/2 reduces update rate). Keep `cpu_load` < ~75%.

### 6. Timing Semantics
`ts_ms`: monotonic window-center (boot relative). `seq`: incremental window counter (gap detection). `win_ms` & `hop_ms`: derived from `n` / overlap. `epoch_ms`: appears once ESPHome time is valid (add a `time:` block); do not compute manually.

### 7. Developer Workflow (PowerShell)
```powershell
esphome config body_sound_sensor.yaml      # Validate YAML & external component
esphome compile body_sound_sensor.yaml     # (First run slower; caches toolchain)
esphome upload body_sound_sensor.yaml      # USB first, OTA thereafter
esphome logs body_sound_sensor.yaml        # Verify MPU wake + JSON updates
./scripts/setup.ps1                        # Guided environment + secrets bootstrap
./scripts/release.ps1 -VersionBump patch   # Release helper (patch|minor|major)
```
Release script: validates config, lightweight Python test (`examples/python/analyze_spectrum.py`), computes next semver, pushes main, triggers GitHub workflow (tag + release). Do not bypass tests when modifying FFT or JSON logic.

### 8. Patterns & Conventions
FFT overlap: `window_shift: 0` means auto 50% (n/2). Preserve this logic unless adding explicit shift configuration—do not inline alternate overlap math.
Memory: avoid per-window allocations; extend fixed arrays if absolutely needed.
I²C address change: only via YAML (`address:`); do not hardcode inside C++.
Band limiting: respect `max_analysis_hz` when computing band centers; clamp to Nyquist.

### 9. Common Validation Checks
Logs must show: MPU wake success, configured `fs`, periodic JSON with all stable fields. Rigid mounting improves SNR; avoid foam (damps high frequencies). If missing sensor data, inspect I²C wiring / address (0x68 vs 0x69).

### 10. Downstream Analysis Workflow
```powershell
pip install -r examples/python/requirements.txt
python examples/python/analyze_spectrum.py --input spectrum.json
```
Use `bands[]` as feature vector; add temporal features via `seq` / `ts_ms` for gap detection and rate normalization.

### 11. Guardrails
Do NOT commit `secrets.yaml` or build artifacts. Preserve all existing JSON field names & ordering. Keep overlap normalization logic unchanged. Seek confirmation before altering public contract or release workflow. Maintain additive schema evolution.

### 12. Safe Change Checklist
1. Modify code (Python/C++)
2. `esphome config body_sound_sensor.yaml`
3. `esphome compile body_sound_sensor.yaml`
4. Upload & verify logs (MPU init + JSON integrity)
5. If JSON changed: confirm backward compatibility & update README example
6. Optional: run `release.ps1` for tagged version

### 13. Quick Decision Guide
Need more resolution? Increase `fft_samples` (monitor CPU). High load? Decrease `sample_frequency` or raise `window_shift`. Missing `epoch_ms`? Ensure `time:` block present—component self-populates later.

Feedback: Ask for clarification before JSON contract, release process, or timing behavior changes.

