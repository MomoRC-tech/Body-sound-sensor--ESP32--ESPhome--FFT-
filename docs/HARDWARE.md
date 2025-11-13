# Hardware Assembly Guide

## Bill of Materials (BOM)

### Essential Components

| Component | Specification | Quantity | Approx. Cost |
|-----------|--------------|----------|--------------|
| ESP32 Development Board | Wemos D1 mini32 or ESP-WROOM-32 | 1 | €5-10 |
| MPU Sensor | MPU-9250, MPU-6500, or MPU-6050 | 1 | €3-8 |
| USB Cable | Micro-USB or USB-C (depending on ESP32 board) | 1 | €2-5 |
| Jumper Wires | Female-to-Female (if using breadboard) | 4 | €1-3 |

### Optional Components

| Component | Purpose | Quantity | Approx. Cost |
|-----------|---------|----------|--------------|
| 3D Printed Enclosure | Protection and mounting | 1 | €2-5 (filament) |
| M3 Screws + Nuts | Rigid mounting | 2-4 | €1 |
| Metal L-Bracket | Alternative rigid mounting | 1 | €2-5 |
| Strong Double-Sided Tape | VHB tape for rigid mounting | 1 roll | €5-10 |

## Wiring Diagram

### Standard I²C Connection

```
ESP32 D1 Mini32          MPU-9250/6500/6050
┌─────────────┐          ┌──────────────┐
│             │          │              │
│   GPIO 21 ──┼─────────►│ SDA          │
│             │          │              │
│   GPIO 22 ──┼─────────►│ SCL          │
│             │          │              │
│       3.3V ──┼─────────►│ VCC          │
│             │          │              │
│        GND ──┼─────────►│ GND          │
│             │          │              │
└─────────────┘          └──────────────┘
```

### Pin Assignments

| Function | ESP32 Pin | MPU Pin | Notes |
|----------|-----------|---------|-------|
| Power | 3.3V | VCC | **Do NOT use 5V!** MPU is 3.3V only |
| Ground | GND | GND | Common ground |
| I²C Data | GPIO 21 | SDA | Standard ESP32 I²C SDA |
| I²C Clock | GPIO 22 | SCL | Standard ESP32 I²C SCL |

### I²C Address Selection

Most MPU boards default to **0x68**. Some boards have an AD0 pin:

- AD0 **LOW** or **not connected**: Address = **0x68**
- AD0 **HIGH** (connected to VCC): Address = **0x69**

If using 0x69, update `body_sound_sensor.yaml`:

```yaml
lambda: |-
  auto mpu_fft = new MPUFftJsonComponent();
  mpu_fft->set_i2c_address(0x69);  # Change to 0x69
  App.register_component(mpu_fft);
```

## Assembly Steps

### Option A: Breadboard Prototype

1. **Insert ESP32** into breadboard
2. **Insert MPU sensor** into breadboard (separate area)
3. **Connect wires** according to diagram above
4. **Connect USB cable** to ESP32
5. **Upload firmware** via USB

✅ **Pros**: Quick, easy to modify  
❌ **Cons**: Not suitable for permanent installation, poor vibration coupling

### Option B: Direct Soldering (Recommended for Production)

1. **Prepare wires**: Cut 4 wires (~10cm each), strip ends
2. **Solder to MPU**: Solder wires to VCC, GND, SDA, SCL pads
3. **Solder to ESP32**: Solder wires to corresponding pins
4. **Add heat shrink** tubing for insulation (optional)
5. **Test connection** before mounting

✅ **Pros**: Reliable, compact, better vibration coupling  
❌ **Cons**: Permanent, harder to debug

### Option C: JST Connectors (Best of Both)

1. **Add JST connectors** to both boards
2. **Create JST cable** with 4 wires
3. **Connect** via JST connectors

✅ **Pros**: Detachable, reliable, professional  
❌ **Cons**: Requires JST crimping tool, more parts

## Mechanical Mounting

### Critical Requirements

The sensor **must be mounted rigidly** to accurately measure structure-borne noise:

- ✅ **Good**: Metal screws through PCB holes into wall/structure
- ✅ **Good**: VHB double-sided tape (3M 5952 or similar)
- ✅ **Good**: Epoxy or strong cyanoacrylate glue
- ⚠️ **Acceptable**: Zip ties (if very tight)
- ❌ **Bad**: Foam tape or foam adhesive
- ❌ **Bad**: Hanging from wires
- ❌ **Bad**: Hot glue (too flexible)

### Mounting Methods

#### Method 1: Screw Mounting (Best)

1. **Mark holes**: Hold sensor against mounting surface
2. **Drill pilot holes**: 2mm for M3 screws
3. **Insert wall plugs** (if mounting to drywall)
4. **Screw sensor down**: Use M3 screws + washers
5. **Verify rigidity**: Sensor should not move when touched

#### Method 2: L-Bracket Mounting

1. **Attach bracket** to sensor with screws
2. **Screw bracket** to wall/structure
3. **Orient sensor** so accelerometer X/Y/Z axes align with expected vibration

#### Method 3: VHB Tape (For Smooth Surfaces)

1. **Clean surface** with isopropyl alcohol
2. **Cut VHB tape** to size (small piece, ~1cm × 2cm)
3. **Apply to sensor PCB** back
4. **Press firmly** onto mounting surface for 30 seconds
5. **Wait 24 hours** for adhesive to cure fully

### Mounting Location Selection

For **pump vibration** monitoring:

- Mount on pipe or wall **near pump**
- Avoid mounting on flexible or damped surfaces
- Closer to source = stronger signal

For **HVAC/fan** monitoring:

- Mount on vent housing or ductwork
- Mount on wall **near** fan unit
- Test multiple locations if signal is weak

### Orientation

The MPU accelerometer measures 3 axes (X, Y, Z). Since we compute the **vector magnitude**, orientation doesn't critically matter, but:

- **Preferred**: Mount with PCB flat against wall (Z-axis perpendicular to wall)
- **Acceptable**: Any orientation, as long as mounting is **rigid**

## Enclosure Design (Optional)

### 3D Printed Enclosure

A simple enclosure can protect the electronics while maintaining rigid coupling:

**Requirements:**
- Thin walls (2-3mm) to minimize damping
- Rigid material (PLA, PETG) - **not** flexible TPU
- Mounting holes for screws
- Cutouts for USB cable and sensor

**Recommended Design:**
- Two-part snap-fit or screw-together case
- Sensor PCB mounts directly to enclosure bottom
- ESP32 PCB mounts with standoffs
- Small ventilation holes (optional, for heat)

**Files:** *(To be added in future update)*

### Minimal Housing

If 3D printing isn't available:

- Use small plastic project box
- Cut holes for USB and wires
- Mount sensor PCB to box with screws
- Mount box to wall with screws or VHB tape

## Testing Assembly

### 1. Visual Inspection

- ✅ All solder joints shiny and smooth
- ✅ No shorts between adjacent pins
- ✅ Wires not under strain

### 2. Continuity Test (Multimeter)

- VCC to VCC: Continuity ✅
- GND to GND: Continuity ✅
- SDA to SDA: Continuity ✅
- SCL to SCL: Continuity ✅
- VCC to GND: **No continuity** ✅

### 3. Power Test

1. Connect USB cable to ESP32
2. ESP32 LED should turn on
3. MPU LED (if present) should turn on

### 4. I²C Scan Test

Upload a simple ESPHome config with I²C scan enabled:

```yaml
i2c:
  sda: 21
  scl: 22
  scan: true
```

Check logs for detected address:
```
[I][i2c:042]: Found i2c device at address 0x68
```

## Safety Warnings

⚠️ **Electrical Safety**
- Use only 3.3V for MPU sensor
- Do not exceed ESP32 pin current limits
- Disconnect power before soldering

⚠️ **Mechanical Safety**
- Ensure mounting is secure (falling hazard)
- Do not over-tighten screws (PCB cracking)
- Drill pilot holes in walls carefully

⚠️ **ESD Protection**
- Handle PCBs by edges
- Use ESD wrist strap when soldering (recommended)
- Store in anti-static bags when not in use

## Troubleshooting Assembly Issues

### No Power to MPU

- Check VCC wire continuity
- Verify 3.3V at MPU VCC pin with multimeter
- Check for shorts

### I²C Not Detected

- Verify SDA/SCL connections
- Check I²C pullup resistors (most MPU boards have them built-in)
- Try different I²C address (0x68 vs 0x69)

### Intermittent Connection

- Check for cold solder joints
- Verify wires are not loose
- Add strain relief to wires

### Poor Vibration Signal

- Check mounting rigidity (sensor must not move)
- Try different mounting location closer to source
- Verify sensor is not on soft/damped surface

## Recommended Tools

### Required
- Soldering iron (if soldering)
- Wire strippers
- Screwdriver (Phillips and/or flathead)
- USB cable for programming

### Optional but Helpful
- Multimeter (continuity and voltage testing)
- Heat shrink tubing + heat gun
- Helping hands / PCB holder
- Flux pen (for easier soldering)
- Drill + 2mm drill bit (for screw mounting)

## Next Steps

Once assembly is complete:

1. ✅ Test I²C communication
2. ✅ Upload ESPHome firmware
3. ✅ Mount sensor in final location
4. ✅ Verify vibration signal in Home Assistant
5. ✅ Proceed to calibration and tuning

See [SETUP.md](SETUP.md) for software setup instructions.
