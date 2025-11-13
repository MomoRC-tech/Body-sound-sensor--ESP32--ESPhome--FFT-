# Quick Start Guide - ESP32 Vibration FFT Analyzer

## Your Configuration

✅ **Hardware**: ESP32-WROOM D1 Mini + MPU6050  
✅ **WiFi Network**: MOMOWLAN  
✅ **I²C Address**: 0x68 (default, will verify on first boot)  
✅ **Mounting**: Solid wall near vibration source  

---

## 🚀 Fast Track Setup (5 Minutes)

### Option 1: Automated Setup (Recommended)

Run the setup script in PowerShell:

```powershell
.\setup.ps1
```

This will:
- ✅ Check Python & ESPHome installation
- ✅ Create `secrets.yaml` with auto-generated API key
- ✅ Validate configuration
- ✅ Optionally compile & upload firmware

**Then just:**
1. Edit WiFi password in `secrets.yaml`
2. Upload to ESP32
3. Done!

---

### Option 2: Manual Setup

```powershell
# 1. Install ESPHome
pip install esphome

# 2. Create secrets file
Copy-Item secrets.yaml.example secrets.yaml

# 3. Generate API key
$bytes = New-Object byte[] 32
[System.Security.Cryptography.RandomNumberGenerator]::Create().GetBytes($bytes)
[Convert]::ToBase64String($bytes)

# 4. Edit secrets.yaml with:
#    - Your WiFi password
#    - Generated API key (from step 3)
#    - An OTA password (your choice)

# 5. Validate config
esphome config body_sound_sensor.yaml

# 6. Upload (connect ESP32 via USB first!)
esphome upload body_sound_sensor.yaml

# 7. Monitor logs
esphome logs body_sound_sensor.yaml
```

---

## 🔌 Hardware Connections

```
MPU6050  →  ESP32-WROOM D1 Mini
────────────────────────────────
VCC      →  3.3V  (NOT 5V!)
GND      →  GND
SDA      →  GPIO 21
SCL      →  GPIO 22
```

⚠️ **Critical**: Use 3.3V, not 5V!

---

## ✅ What to Expect

### During Upload
```
Connecting...
Writing at 0x00001000... (10%)
...
Hard resetting via RTS pin...
```

### In Logs (esphome logs ...)
```
[I][wifi:051]: WiFi Connected!
[I][i2c:068]: Found i2c device at address 0x68  ← MPU6050 detected!
[I][MPU_FFT:123]: MPU6050 FFT Component initialized
[I][MPU_FFT:124]: Sample rate: 1000 Hz, FFT size: 512
[sensor:123]: 'Body Sound Vibration RMS': Got value 0.0023
[text_sensor:047]: 'Body Sound Spectrum JSON': Got value {"fs":1000.0,...}
```

### In Home Assistant

**Settings → Devices & Services → ESPHome**

You should see device with 3 sensors:
1. **Body Sound Vibration RMS** - Overall vibration level (0.001-0.05g typical)
2. **Body Sound FFT CPU Load** - ESP32 CPU usage (should be 30-50%)
3. **Body Sound Spectrum JSON** - Full spectral data

---

## 🔧 Troubleshooting

### Issue: No I²C device found at 0x68

**Solution 1**: Try address 0x69
```yaml
# Edit body_sound_sensor.yaml, line ~48:
mpu_fft->set_i2c_address(0x69);  # Change to 0x69
```

**Solution 2**: Check wiring
- Verify SDA/SCL connections
- Ensure 3.3V power (measure with multimeter)
- Check GND connection

### Issue: WiFi not connecting

Check `secrets.yaml`:
```yaml
wifi_ssid: "MOMOWLAN"          # Must match exactly (case-sensitive!)
wifi_password: "actual_password" # No quotes inside quotes!
```

### Issue: COM port not detected

1. Install CP210x USB driver: [Download here](https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers)
2. Try different USB cable (some are charge-only)
3. Try different USB port on PC

---

## 📊 Testing Vibration Detection

Once running, test by tapping the sensor:

```powershell
esphome logs body_sound_sensor.yaml
```

**Tap the MPU6050** → RMS value should spike:
```
[sensor:123]: 'Body Sound Vibration RMS': 0.0023  ← idle
[sensor:123]: 'Body Sound Vibration RMS': 0.0456  ← tapped!
[sensor:123]: 'Body Sound Vibration RMS': 0.0019  ← back to idle
```

---

## 🏠 Adding to Home Assistant

1. **Settings** → **Devices & Services**
2. ESPHome device should auto-discover
3. Click **Configure**
4. Enter API key from `secrets.yaml`
5. Done! 🎉

---

## 📍 Final Installation

Once working:

1. **Mount MPU6050 rigidly** on solid wall near vibration source
   - Use M3 screws through mounting holes, OR
   - VHB double-sided tape (3M 5952)
   - **Avoid**: Foam tape, hot glue

2. **Mount ESP32 nearby** (can use cable ties or small box)

3. **Permanent power**: 5V USB adapter

4. **Verify signal**: Check vibration RMS changes when source is active

---

## 🎯 Expected Performance

| Metric | Value |
|--------|-------|
| **Idle RMS** | 0.001 - 0.005 g |
| **Active RMS** | 0.01 - 0.05 g (depends on source) |
| **CPU Load** | 30 - 50% |
| **Update Rate** | ~4 spectra/second |
| **Frequency Range** | 0 - 500 Hz |

---

## 📚 Next Steps

Once installed and working:

1. ✅ **Collect baseline data** (24 hours)
2. ✅ **Analyze spectra** with Python tools: `examples/python/analyze_spectrum.py`
3. ✅ **Create automations** in Home Assistant
4. ✅ **Set up alerts** for unusual vibration patterns

---

## 📖 Full Documentation

- **Detailed Setup**: `docs/SETUP.md`
- **Hardware Guide**: `docs/HARDWARE.md`
- **Main README**: `README.md`

---

## 🆘 Need Help?

1. Check logs: `esphome logs body_sound_sensor.yaml`
2. Enable debug logging:
   ```yaml
   logger:
     level: DEBUG
   ```
3. Review `docs/SETUP.md` troubleshooting section
4. Open GitHub issue with logs

---

**Ready to start?** Run `.\setup.ps1` now! 🚀
