# Detailed Setup Guide for ESP32 Vibration FFT Analyzer

## Hardware Configuration

### Your Specific Setup
- **MCU**: ESP32-WROOM D1 Mini
- **Sensor**: MPU6050 3-axis accelerometer
- **I²C Address**: 0x68 (default, to be verified during setup)
- **WiFi Network**: MOMOWLAN
- **Mounting**: Solid wall near vibration source

---

## Prerequisites

### Software Requirements

1. **Python 3.8+** (for ESPHome)
   ```powershell
   python --version
   ```

2. **ESPHome** (install via pip)
   ```powershell
   pip install esphome
   ```

3. **USB Driver** for ESP32
   - Download: [CP210x USB to UART Bridge](https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers)
   - Install and restart

4. **Git** (for version control)
   ```powershell
   git --version
   ```

### Hardware Requirements

- ESP32-WROOM D1 Mini board
- MPU6050 sensor module
- 4x female-to-female jumper wires
- Micro-USB cable
- Mounting hardware (screws or VHB tape)

---

## Step-by-Step Setup

### 1. Verify Repository Structure

Navigate to your project directory:

```powershell
cd "C:\Users\Momo\Documents\30_Coding\01_github local\Body-sound-sensor--ESP32--ESPhome--FFT-"
```

Verify files exist:
```powershell
ls
```

You should see:
- `body_sound_sensor.yaml`
- `mpu_fft_json.h`
- `secrets.yaml.example`

### 2. Create Secrets File

Copy the template:
```powershell
Copy-Item secrets.yaml.example secrets.yaml
```

Generate an API encryption key:
```powershell
# Using OpenSSL (if installed)
openssl rand -base64 32

# OR using PowerShell
$bytes = New-Object byte[] 32
[System.Security.Cryptography.RandomNumberGenerator]::Create().GetBytes($bytes)
[Convert]::ToBase64String($bytes)
```

Edit `secrets.yaml` with your actual credentials:
```yaml
# ESP32 Vibration Sensor - WiFi and Security Configuration

# WiFi Configuration
wifi_ssid: "MOMOWLAN"
wifi_password: "your_actual_wifi_password_here"

# API encryption key
api_encryption_key: "generated_key_from_above"

# OTA password
ota_password: "choose_a_secure_password"
```

⚠️ **IMPORTANT**: Never commit `secrets.yaml` to git! It's already in `.gitignore`.

### 3. Wire the Hardware

Connect MPU6050 to ESP32-WROOM D1 Mini:

| MPU6050 Pin | ESP32 Pin | Wire Color (suggested) |
|-------------|-----------|------------------------|
| VCC         | 3.3V      | Red                    |
| GND         | GND       | Black                  |
| SDA         | GPIO 21   | Yellow                 |
| SCL         | GPIO 22   | Green                  |

**Important Notes:**
- Use **3.3V**, NOT 5V (MPU6050 is 3.3V only)
- Keep wires short (~10cm) for reliable I²C communication
- Double-check connections before powering on

### 4. Connect ESP32 to Computer

1. Connect ESP32 to your PC via USB
2. Wait for driver installation (if first time)
3. Note the COM port (e.g., COM3, COM5)

Check COM port in PowerShell:
```powershell
Get-PnpDevice -Class Ports | Where-Object {$_.FriendlyName -like "*USB*"}
```

Or check in Device Manager: `Win + X` → Device Manager → Ports (COM & LPT)

### 5. Validate Configuration

Before uploading, validate the YAML:

```powershell
esphome config body_sound_sensor.yaml
```

Expected output:
```
INFO Reading configuration body_sound_sensor.yaml...
INFO Configuration is valid!
```

If you see errors, check:
- `secrets.yaml` exists and has correct format
- All required fields are filled
- YAML indentation is correct (use spaces, not tabs)

### 6. Compile Firmware

Compile the firmware (optional test before upload):

```powershell
esphome compile body_sound_sensor.yaml
```

This will:
- Download Arduino framework and libraries
- Compile the C++ code
- Generate firmware binary

**First compile takes ~5-10 minutes**. Subsequent compiles are much faster.

### 7. Upload to ESP32

Upload firmware via USB:

```powershell
esphome upload body_sound_sensor.yaml
```

Select the correct COM port when prompted.

Monitor the upload progress. You should see:
```
Uploading...
Writing at 0x00001000... (10%)
...
Hard resetting via RTS pin...
```

### 8. Monitor Logs

After upload, immediately check logs:

```powershell
esphome logs body_sound_sensor.yaml
```

**What to look for:**

✅ **Success Indicators:**
```
[I][app:102]: ESPHome version compiled on Nov 13 2025
[I][app:104]: Project: brunnen_vibration_fft
[C][wifi:038]: Setting up WiFi...
[C][wifi:051]: WiFi Connected!
[I][i2c.arduino:068]: Found i2c device at address 0x68
[I][MPU_FFT:123]: MPU6050 FFT Component initialized successfully
[I][MPU_FFT:124]: Sample rate: 1000 Hz, FFT size: 512, Bands: 16
```

❌ **Common Issues:**

**WiFi not connecting:**
```
[W][wifi:386]: WiFi Connection failed! SSID: 'MOMOWLAN'
```
→ Check `secrets.yaml` WiFi password

**I²C sensor not found:**
```
[W][i2c.arduino:071]: No i2c device found at address 0x68
```
→ Check wiring, try address 0x69

**MPU sensor failed:**
```
[E][MPU_FFT:034]: Failed to wake up MPU6050 sensor!
```
→ Check power (3.3V), verify I²C connections

### 9. Verify I²C Address

If sensor is not detected at 0x68, try 0x69:

1. Stop logging (Ctrl+C)
2. Edit `body_sound_sensor.yaml`:
   ```yaml
   mpu_fft->set_i2c_address(0x69);  # Change to 0x69
   ```
3. Re-upload and check logs

To auto-detect I²C devices, check the log output for:
```
[I][i2c.arduino:068]: Found i2c device at address 0xXX
```

### 10. Test Vibration Detection

With logs running, tap the sensor gently:

```powershell
esphome logs body_sound_sensor.yaml
```

You should see RMS values change:
```
[sensor:123]: 'Brunnen Vibration RMS': Got value 0.0023
[sensor:123]: 'Brunnen Vibration RMS': Got value 0.0456  # <- increased when tapped
[sensor:123]: 'Brunnen Vibration RMS': Got value 0.0018
```

Also check for JSON spectrum updates:
```
[text_sensor:047]: 'Brunnen Spectrum JSON': Got value {"fs":1000.0,"n":512,"bin_hz":1.953,"rms":0.0234,"peak_hz":48.2,"bands":[...]}
```

### 11. Add to Home Assistant

1. Open Home Assistant
2. Go to **Settings** → **Devices & Services**
3. Look for "ESPHome" discovered device
4. Click **Configure**
5. Enter the API encryption key from `secrets.yaml`
6. The device should appear with 3 sensors:
   - Brunnen Vibration RMS
   - Brunnen FFT CPU Load
   - Brunnen Spectrum JSON

### 12. Mount Sensor (Final Step)

Once confirmed working:

1. **Choose mounting location:**
   - Solid wall near vibration source (pump, pipe)
   - Within WiFi range
   - Accessible for maintenance

2. **Mount rigidly:**
   - **Option A (Recommended)**: 2x M3 screws through MPU6050 mounting holes
   - **Option B**: VHB double-sided tape (3M 5952 or similar)
   - **Avoid**: Foam tape, hot glue, or flexible mounting

3. **Secure ESP32 nearby:**
   - Can use cable ties or small project box
   - Ensure USB cable doesn't strain connection

4. **Power permanently:**
   - USB power adapter (5V/1A minimum)
   - Or existing USB power source

---

## Post-Installation Configuration

### Optimize Performance

Monitor CPU Load in Home Assistant. If consistently >70%:

1. Edit `mpu_fft_json.h`:
   ```cpp
   static const float    SAMPLE_FREQUENCY  = 500.0f;  // Reduce from 1000
   static const uint16_t FFT_SAMPLES       = 256;     // Reduce from 512
   ```

2. Re-compile and upload:
   ```powershell
   esphome upload body_sound_sensor.yaml
   ```

### Calibrate Thresholds

1. Run pump/fan manually
2. Note typical RMS values in Home Assistant
3. Create automations based on thresholds

Example: Pump detection
- **Idle**: RMS < 0.005g
- **Pump active**: RMS > 0.02g

---

## Troubleshooting

### Issue: ESPHome Not Found

```powershell
# Install/update ESPHome
pip install --upgrade esphome

# Verify installation
esphome version
```

### Issue: USB Driver Not Working

1. Download [CP210x drivers](https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers)
2. Install for Windows
3. Restart computer
4. Reconnect ESP32

### Issue: Cannot Find COM Port

```powershell
# List all COM ports
mode
```

Try different USB port or cable.

### Issue: Upload Failed

```powershell
# Use specific COM port
esphome upload body_sound_sensor.yaml --device COM3
```

### Issue: Home Assistant Not Discovering Device

1. Check both devices on same WiFi network
2. Check firewall settings (port 6053)
3. Manually add via IP:
   - Settings → Integrations → Add Integration → ESPHome
   - Enter ESP32 IP address from logs

### Issue: Noisy RMS Readings

Edit `mpu_fft_json.h`:
```cpp
static const float    DC_ALPHA = 0.001f;  // Stronger high-pass (was 0.01)
```

Or add averaging in Home Assistant sensor configuration.

---

## Over-The-Air (OTA) Updates

After first USB upload, subsequent updates can be wireless:

```powershell
esphome upload body_sound_sensor.yaml
```

ESPHome will auto-detect and use OTA instead of USB.

---

## Next Steps

✅ After successful installation:

1. **Collect baseline data** (24 hours)
2. **Analyze spectrum patterns** using `examples/python/analyze_spectrum.py`
3. **Create Home Assistant automations**
4. **Set up alerts** for abnormal vibration

See main README.md for advanced usage and ML integration.

---

## Support

If you encounter issues:

1. Check logs: `esphome logs body_sound_sensor.yaml`
2. Review [ESPHome documentation](https://esphome.io/)
3. Check GitHub Issues
4. Enable debug logging in YAML:
   ```yaml
   logger:
     level: DEBUG
   ```

---

**Last Updated**: November 13, 2025
