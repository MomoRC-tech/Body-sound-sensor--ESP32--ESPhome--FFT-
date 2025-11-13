# Changelog

## v0.9.6 - 2025-11-13

- docs: refresh AI coding agent guide (`.github/copilot-instructions.md`) for concise, actionable repo-specific instructions
- docs(readme): unify all collapsible summaries with blue, bold styling for better UI visibility

## v0.9.5 - 2025-11-13

- chore(release): prepare v0.9.4 changelog and README version bump


## v0.9.4 - 2025-11-13

- feat(json): add timing metadata (ts_ms, win_ms, hop_ms, seq, epoch_ms) for ML alignment and event correlation
- feat(diagnostics): expose window_shift as diagnostic sensor (samples) for overlap visibility
- feat(python): time-series plot helper (plot_band_over_time) + accumulation demo in analyze_spectrum.py
- docs: update README JSON example & add Timing Metadata section; add AI agent instructions file
- internal: optional SNTP time integration (time_id) to enable epoch_ms without breaking existing parsers

## v0.9.3 - 2025-11-13

- feat(json): add timing metadata fields `ts_ms`, `win_ms`, `hop_ms`, `seq`, and optional `epoch_ms` to spectrum JSON for ML alignment and historical correlation
- feat(diagnostics): expose `window_shift` as diagnostic sensor (samples)
- feat(python): add time-series plot helper and accumulation demo for band energy vs time
- docs: update README with new JSON example and Timing Metadata section; add agent instructions file




## v0.9.2 - 2025-11-13

- fix: restore MPUFftJsonComponent structure and namespace; clean JSON/diagnostics and FFT processing
- docs(diagnostics): document diagnostic sensors and YAML tuning; fix default max_analysis_hz=300Hz
- feat(diagnostics): expose bin_hz, sample_frequency, fft_samples, fft_bands, max_analysis_hz as diagnostic sensors; fix process_window_ integrity
- feat(config): make sample_frequency, fft_samples, fft_bands, window_shift, dc_alpha, load_window_us configurable via YAML; dynamic FFT buffers
- feat(mpu_fft_json): make max_analysis_hz configurable via YAML; docs and example updated
- feat: include band_low/high/center and max_analysis_hz in spectrum JSON; update example and docs
- docs: update README


## v0.9.1 - 2025-11-13

- chore: remove root PowerShell scripts (moved to scripts/)
- chore: move PowerShell scripts to scripts/; remove release/compile artifacts; update docs and .gitignore
- chore: remove duplicate root mpu_fft_json.h; keep canonical file under custom_components/mpu_fft_json/
- chore(release): update CHANGELOG and README for v0.9.0
- ci(release): skip tag creation when tag already exists
- chore(release): update CHANGELOG and README for v0.9.0
- chore(docs): generalize sensor names from Brunnen to Body Sound across docs and examples


## v0.9.0 - 2025-11-13

- ci(release): skip tag creation when tag already exists
- chore(release): update CHANGELOG and README for v0.9.0
- chore(docs): generalize sensor names from Brunnen to Body Sound across docs and examples


## v0.9.0 - 2025-11-13

- chore(docs): generalize sensor names from Brunnen to Body Sound across docs and examples


## v0.9.0 - 2025-11-13

- ci: make clang-format non-blocking; release: support explicit version override
- ci: fix API key generation and header path in esphome-ci workflow
- docs(readme): link docs/*.md with clickable markdown
- chore: remove trailing whitespace in header
- docs(readme): simplify install/upload, fix CI badge note, link docs, update version to v0.0.2


## v0.0.2 - 2025-11-13

- ci(release): remove duplicate header asset to fix 404 on upload


## v0.0.1 - 2025-11-13

- Initial release


All notable changes to this project will be documented in this file.

This project adheres to Semantic Versioning.
