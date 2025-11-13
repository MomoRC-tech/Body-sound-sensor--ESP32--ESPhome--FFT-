# Project Implementation Checklist

## ✅ Phase 1: Initial Setup (Completed)

- [x] Project structure created
- [x] ESPHome YAML configuration (body_sound_sensor.yaml)
- [x] C++ FFT component implementation (mpu_fft_json.h)
- [x] Secrets template with MOMOWLAN WiFi
- [x] CI/CD pipeline configured
- [x] Documentation (README, SETUP, HARDWARE, QUICKSTART)
- [x] Python analysis examples
- [x] Automated setup script (setup.ps1)

**Hardware Configured:**
- ESP32-WROOM D1 Mini
- MPU6050 sensor
- I²C address: 0x68 (to be verified)

---

## 📋 Phase 2: Hardware Assembly & First Boot

### Tasks
- [ ] Wire MPU6050 to ESP32 (VCC→3.3V, GND→GND, SDA→21, SCL→22)
- [ ] Install CP210x USB drivers (if needed)
- [ ] Connect ESP32 to PC via USB
- [ ] Verify COM port detection

### Validation
- [ ] Visual inspection of connections
- [ ] Continuity test with multimeter (optional)
- [ ] Power LED on both boards

**Status**: ⏳ Waiting for hardware assembly

---

## 📋 Phase 3: Software Configuration

### Tasks
- [ ] Run `.\setup.ps1` OR manually create secrets.yaml
- [ ] Generate API encryption key
- [ ] Edit secrets.yaml with:
  - [ ] WiFi password for MOMOWLAN
  - [ ] Generated API key
  - [ ] OTA password
- [ ] Install ESPHome: `pip install esphome`

### Validation
- [ ] `esphome config body_sound_sensor.yaml` passes
- [ ] No placeholder values in secrets.yaml
- [ ] ESPHome version check: `esphome version`

**Status**: ⏳ Ready to configure

---

## 📋 Phase 4: First Upload & Testing

### Tasks
- [ ] Compile firmware: `esphome compile body_sound_sensor.yaml`
- [ ] Upload via USB: `esphome upload body_sound_sensor.yaml`
- [ ] Monitor logs: `esphome logs body_sound_sensor.yaml`

### Expected Log Output
- [ ] WiFi connected to MOMOWLAN
- [ ] I²C device found at 0x68 (or 0x69)
- [ ] MPU6050 FFT Component initialized
- [ ] Sample rate: 1000 Hz, FFT size: 512
- [ ] RMS sensor publishing values
- [ ] JSON spectrum updates

### If I²C Not Detected
- [ ] Check wiring (especially SDA/SCL)
- [ ] Verify 3.3V power with multimeter
- [ ] Try I²C address 0x69 in YAML
- [ ] Reseat connections

**Status**: ⏳ Ready to upload

---

## 📋 Phase 5: Home Assistant Integration

### Tasks
- [ ] Open Home Assistant
- [ ] Go to Settings → Devices & Services
- [ ] Look for ESPHome auto-discovery
- [ ] Configure with API key from secrets.yaml
- [ ] Verify 3 sensors appear:
  - [ ] Brunnen Vibration RMS
  - [ ] Brunnen FFT CPU Load
  - [ ] Brunnen Spectrum JSON

### Validation
- [ ] All sensors showing values
- [ ] RMS updates regularly (~1/sec)
- [ ] CPU Load < 70%
- [ ] JSON contains valid data

**Status**: ⏳ Pending Phase 4

---

## 📋 Phase 6: Vibration Testing (Bench)

### Tasks
- [ ] Monitor logs in real-time
- [ ] Tap MPU6050 sensor gently
- [ ] Observe RMS spike in logs
- [ ] Check JSON spectrum changes
- [ ] Verify peak_hz shifts with tap frequency

### Expected Behavior
- [ ] Idle RMS: 0.001 - 0.005g
- [ ] Tap RMS: 0.02 - 0.10g
- [ ] CPU Load: 30 - 60%
- [ ] Spectrum updates ~4/sec

**Status**: ⏳ Pending Phase 5

---

## 📋 Phase 7: Physical Installation

### Site Selection
- [ ] Choose mounting location (solid wall near vibration source)
- [ ] Verify WiFi signal strength at location
- [ ] Ensure accessibility for maintenance
- [ ] Check proximity to USB power

### Mounting
- [ ] Mount MPU6050 rigidly:
  - [ ] Option A: M3 screws through mounting holes
  - [ ] Option B: VHB tape (3M 5952 or similar)
- [ ] Secure ESP32 nearby (cable ties or box)
- [ ] Route USB cable for permanent power
- [ ] Verify sensor doesn't move when touched

### Power
- [ ] Connect to 5V USB power adapter
- [ ] Verify stable power (no brownouts in logs)
- [ ] Test OTA updates work from installed location

**Status**: ⏳ Pending Phase 6

---

## 📋 Phase 8: Calibration & Baseline

### Tasks
- [ ] Record RMS values during known states:
  - [ ] Background/idle (nothing running)
  - [ ] Pump active
  - [ ] Fan active
  - [ ] Other sources (if applicable)
- [ ] Collect JSON spectra for 24 hours
- [ ] Run Python analyzer: `python examples/python/analyze_spectrum.py`
- [ ] Visualize spectrum differences between states

### Thresholds to Determine
- [ ] Idle threshold (e.g., RMS < 0.005g)
- [ ] Pump active threshold (e.g., RMS > 0.02g)
- [ ] Fan active threshold
- [ ] Frequency bands characteristic of each device

**Status**: ⏳ Pending Phase 7

---

## 📋 Phase 9: Home Assistant Automations

### Basic Automations
- [ ] Create binary sensor for "Pump Active" (RMS threshold)
- [ ] Create binary sensor for "Fan Active"
- [ ] Set up notification when vibration exceeds threshold
- [ ] Log to database for trending

### Example Automation (Pump Detection)
```yaml
binary_sensor:
  - platform: template
    sensors:
      pump_active:
        friendly_name: "Well Pump Active"
        value_template: >
          {{ states('sensor.brunnen_vibration_rms') | float > 0.02 }}
```

- [ ] Test automations with manual pump/fan operation
- [ ] Tune thresholds to avoid false positives
- [ ] Set up alerts/notifications

**Status**: ⏳ Pending Phase 8

---

## 📋 Phase 10: Advanced Analysis (Optional)

### Machine Learning
- [ ] Set up spectrum logging to CSV
- [ ] Collect labeled training data (pump_on, fan_on, idle)
- [ ] Train classifier (scikit-learn, TensorFlow)
- [ ] Deploy model for real-time classification

### Node-RED Integration
- [ ] Create Node-RED flow to parse JSON
- [ ] Implement complex logic (multi-device detection)
- [ ] Create dashboard with spectrum visualization

### Anomaly Detection
- [ ] Establish baseline spectral patterns
- [ ] Detect deviations (mechanical wear, bearing issues)
- [ ] Alert on unusual vibration signatures

**Status**: ⏳ Future enhancement

---

## 📋 Phase 11: Optimization & Tuning

### Performance Tuning
- [ ] Monitor CPU Load over 24 hours
- [ ] If CPU > 70%, reduce:
  - [ ] SAMPLE_FREQUENCY (1000 → 500 Hz)
  - [ ] FFT_SAMPLES (512 → 256)
  - [ ] FFT_BANDS (16 → 8)
- [ ] Re-test after changes

### Signal Quality
- [ ] Check for noisy RMS readings
- [ ] Adjust DC_ALPHA if needed (0.01 → 0.001 for stronger high-pass)
- [ ] Add averaging filters in Home Assistant if needed

### Documentation
- [ ] Document final thresholds
- [ ] Add device-specific mounting notes
- [ ] Update README with findings

**Status**: ⏳ Pending Phase 8

---

## 📋 Phase 12: Maintenance & Monitoring

### Ongoing Tasks
- [ ] Weekly check of CPU Load
- [ ] Monthly review of vibration trends
- [ ] Quarterly check of mounting rigidity
- [ ] Update firmware as needed (OTA)

### Backup & Version Control
- [ ] Commit final configuration to GitHub
- [ ] Document calibration values
- [ ] Export Home Assistant automations
- [ ] Keep backup of secrets.yaml (secure location!)

**Status**: ⏳ Ongoing

---

## 🎯 Success Criteria

### Minimum Viable Product (MVP)
- ✅ ESP32 boots and connects to WiFi
- ✅ MPU6050 detected on I²C bus
- ✅ RMS values update in Home Assistant
- ✅ CPU Load < 80%
- ✅ JSON spectrum contains valid data

### Full Deployment
- ✅ Sensor mounted in final location
- ✅ Reliable WiFi connection
- ✅ Accurate device detection (pump/fan)
- ✅ Automations working as expected
- ✅ No false positives/negatives

### Advanced Features
- ✅ ML-based classification working
- ✅ Anomaly detection operational
- ✅ Dashboard with visualizations
- ✅ Multi-sensor deployment (if desired)

---

## 📊 Progress Tracker

| Phase | Status | Date Completed |
|-------|--------|----------------|
| 1. Initial Setup | ✅ Complete | Nov 13, 2025 |
| 2. Hardware Assembly | ⏳ Pending | |
| 3. Software Config | ⏳ Pending | |
| 4. First Upload | ⏳ Pending | |
| 5. HA Integration | ⏳ Pending | |
| 6. Vibration Testing | ⏳ Pending | |
| 7. Physical Install | ⏳ Pending | |
| 8. Calibration | ⏳ Pending | |
| 9. Automations | ⏳ Pending | |
| 10. Advanced Analysis | ⏳ Optional | |
| 11. Optimization | ⏳ Pending | |
| 12. Maintenance | ⏳ Ongoing | |

---

## 🆘 Known Issues & Solutions

### Issue: High CPU Load (>80%)
**Solution**: Reduce SAMPLE_FREQUENCY or FFT_SAMPLES in mpu_fft_json.h

### Issue: Noisy RMS Readings
**Solution**: Adjust DC_ALPHA, check mounting rigidity

### Issue: WiFi Disconnections
**Solution**: Check signal strength, reduce update rate, add reconnect logic

### Issue: I²C Communication Errors
**Solution**: Shorter wires, reduce I²C frequency to 100kHz, check pullup resistors

---

## 📞 Support Resources

- **Documentation**: README.md, docs/SETUP.md, docs/HARDWARE.md
- **ESPHome Docs**: https://esphome.io/
- **GitHub Issues**: (project repository)
- **Home Assistant Forum**: https://community.home-assistant.io/

---

**Next Action**: Proceed to Phase 2 (Hardware Assembly)

**Need Help?** Run `.\setup.ps1` to begin automated setup!
