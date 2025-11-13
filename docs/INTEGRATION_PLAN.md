# Body Sound Sensor – Integration Blueprint

This file duplicates `body_sound_integration_plan.md` for stable documentation under `docs/` so downstream tools, automation, and contributors can rely on its location.

(Original source retained at repository root for now; prefer editing this copy going forward.)

<!-- START PLAN COPY -->

> Project: `Body-sound-sensor--ESP32--ESPhome--FFT-`  
> Current release: **v0.9.6** (after docs updates)  
> Goal: Blueprint for HA dashboards, Node‑RED, InfluxDB, Grafana, ML.

---

## 1. Existing Implementation
- Custom ESPHome component `mpu_fft_json` (C++ + Python codegen) performing sampling, HPF, FFT (Hamming), band aggregation, RMS, CPU load.
- Spectrum + metadata published via JSON text sensor; numeric sensors for RMS & CPU; diagnostics optional.
- JSON fields: `schema_version`, `fs`, `n`, `bin_hz`, `rms`, `peak_hz`, `max_analysis_hz`, timing (`ts_ms`, `win_ms`, `hop_ms`, `seq`, optional `epoch_ms`), band arrays (`bands`, `band_center`, `band_low`, `band_high`).
- Docs and CI workflows present; Python example for spectrum analysis.

## 2. Next Goals
1. HA dashboards (RMS, CPU).  
2. Node‑RED parsing + InfluxDB storage.  
3. Grafana visualizations (trend, snapshot, heatmap).  
4. Optional ML classification later.

## 3. System Architecture Snapshot
ESP32 (DSP node) -> HA (entities + events) -> Node‑RED (parse + enrich) -> InfluxDB (time‑series) -> Grafana (dashboards) -> Optional ML loop.

## 4. Firmware Guidance
- Treat JSON as the stable contract; only additive changes.  
- Added `schema_version: 1` for downstream version gating.  
- Future optional additions: saturation flag, noise metrics.

## 5. Home Assistant Tasks
- Confirm entities present (RMS, CPU, Spectrum JSON).  
- Add RMS & CPU charts (History or ApexCharts).  
- Create first threshold `binary_sensor` (e.g. pump active).  
- Avoid per‑band sensors—use JSON for full spectrum.

## 6. Node‑RED Flow Essentials
- Event source: HA state change of spectrum JSON.  
- Filter unknown/unavailable.  
- Function: parse JSON, map `bands[]` -> `band_0..band_N`, include RMS, peak_hz, fs, n, max_analysis_hz, seq, ts_ms, epoch_ms, cpu_load (from separate sensor).  
- Output payload to InfluxDB.

## 7. InfluxDB Storage
- Measurement: `body_sound`.  
- Fields: `band_0..band_15`, `rms`, `peak_hz`, `fs`, `fft_size`, optional `cpu_load`.  
- Tags: `sensor`, `location`.  
- Retention: e.g. 30d; downsample if needed.

## 8. Grafana Panels
- RMS trend, CPU trend, latest spectrum (bars), band heatmap (spectrogram).  
- Optional variables for sensor & time range.

## 9. ML (Later)
- Label spectra (pump_on / idle / etc.).  
- Train classifier (scikit‑learn).  
- Deploy scoring in Node‑RED or separate microservice; publish result back to HA.

## 10. Compact Checklist
```
Home Assistant:
 [ ] RMS chart  [ ] CPU chart  [ ] pump_active binary sensor
Node-RED:
 [ ] State event node -> parse -> InfluxDB out
InfluxDB:
 [ ] Measurement + retention
Grafana:
 [ ] RMS panel [ ] CPU panel [ ] Spectrum bars [ ] Heatmap
Firmware (optional):
 [x] schema_version field
ML (later):
 [ ] Data labeling [ ] Model training [ ] HA integration
```

<!-- END PLAN COPY -->
