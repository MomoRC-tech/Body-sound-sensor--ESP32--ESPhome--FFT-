#pragma once

#include "esphome.h"
#include "arduinoFFT.h"
#include <cmath>

using namespace esphome;

// ============================================================================
// CONFIGURATION CONSTANTS (Tunable Parameters)
// ============================================================================

// Sampling configuration
static const float    SAMPLE_FREQUENCY  = 1000.0f;     // Hz
static const uint16_t FFT_SAMPLES       = 512;         // Must be power of 2
static const uint32_t SAMPLE_PERIOD_US  = (uint32_t)(1000000.0f / SAMPLE_FREQUENCY);

// FFT configuration
static const uint8_t  FFT_BANDS         = 16;          // Number of frequency bands
static const uint16_t WINDOW_SHIFT      = FFT_SAMPLES / 2;  // 50% overlap

// High-pass filter coefficient for DC removal
static const float    DC_ALPHA          = 0.01f;

// CPU load measurement window (1 second)
static const uint32_t LOAD_WINDOW_US    = 1000000;

// MPU-9250/6500/6050 Register addresses
static const uint8_t  MPU_PWR_MGMT_1    = 0x6B;
static const uint8_t  MPU_ACCEL_XOUT_H  = 0x3B;

// Accelerometer scaling for MPU6050 (±2g range, 16384 LSB/g)
static const float    ACCEL_SCALE       = 1.0f / 16384.0f;

// ============================================================================
// MPU FFT JSON Component Class
// ============================================================================

namespace mpu_fft_json {

class MPUFftJsonComponent : public Component, public i2c::I2CDevice {
public:
  // Setter methods for ESPHome external component pattern
  void set_rms_sensor(sensor::Sensor *sensor) { rms_sensor_ = sensor; }
  void set_cpu_load_sensor(sensor::Sensor *sensor) { cpu_load_sensor_ = sensor; }
  void set_spectrum_text_sensor(text_sensor::TextSensor *sensor) { spectrum_text_ = sensor; }

  MPUFftJsonComponent() = default;

  void setup() override {
    ESP_LOGD("MPU_FFT", "Setting up MPU6050 FFT Component...");

    // Wake up MPU6050 (clear sleep bit in PWR_MGMT_1)
    if (!this->write_byte(MPU_PWR_MGMT_1, 0x00)) {
      ESP_LOGE("MPU_FFT", "Failed to wake up MPU6050 sensor!");
      this->mark_failed();
      return;
    }

    esphome::delay(100);  // Give sensor time to wake up

    // Initialize timing and state
    last_sample_us_ = esphome::micros();
    sample_index_ = 0;
    dc_avg_ = 1.0f;  // Initialize to ~1g (gravity)

    // Initialize CPU load tracking
    load_window_start_us_ = last_sample_us_;
    busy_time_us_ = 0;

    // Clear buffers
    for (uint16_t i = 0; i < FFT_SAMPLES; i++) {
      vReal_[i] = 0.0;
      vImag_[i] = 0.0;
    }

    ESP_LOGI("MPU_FFT", "MPU FFT Component initialized successfully");
    ESP_LOGI("MPU_FFT", "Sample rate: %.0f Hz, FFT size: %d, Bands: %d",
             SAMPLE_FREQUENCY, FFT_SAMPLES, FFT_BANDS);
  }

  void loop() override {
    uint32_t loop_start = esphome::micros();
    uint32_t now = esphome::micros();

    // Maintain constant sampling rate
    if ((now - last_sample_us_) >= SAMPLE_PERIOD_US) {
      last_sample_us_ = now;
      sample_once_();
    }

    // Update CPU load periodically
    uint32_t window_elapsed = now - load_window_start_us_;
    if (window_elapsed >= LOAD_WINDOW_US) {
      float load = (float)busy_time_us_ / (float)window_elapsed;
      float cpu_load = load * 100.0f;

      if (cpu_load_sensor_ && (cpu_load_sensor_->has_state() || cpu_load > 1.0f)) {
        cpu_load_sensor_->publish_state(cpu_load);
      }

      // Reset for next window
      busy_time_us_ = 0;
      load_window_start_us_ = now;
    }

    // Accumulate busy time
    uint32_t loop_end = esphome::micros();
    busy_time_us_ += (loop_end - loop_start);
  }

protected:
  // Sensor pointers (set by ESPHome codegen)
  sensor::Sensor *rms_sensor_{nullptr};
  sensor::Sensor *cpu_load_sensor_{nullptr};
  text_sensor::TextSensor *spectrum_text_{nullptr};

  // FFT buffers
  double vReal_[FFT_SAMPLES];
  double vImag_[FFT_SAMPLES];

  // Sampling state
  uint16_t sample_index_ = 0;
  uint32_t last_sample_us_ = 0;
  float dc_avg_ = 1.0f;

  // CPU load tracking
  uint32_t load_window_start_us_ = 0;
  uint32_t busy_time_us_ = 0;

  // ========================================================================
  // Read accelerometer data in g units
  // ========================================================================
  bool read_accel_g_(float &ax, float &ay, float &az) {
    uint8_t data[6];

    if (!this->read_bytes(MPU_ACCEL_XOUT_H, data, 6)) {
      ESP_LOGW("MPU_FFT", "Failed to read accelerometer data");
      return false;
    }

    // Combine high and low bytes (big-endian)
    int16_t raw_ax = (int16_t)((data[0] << 8) | data[1]);
    int16_t raw_ay = (int16_t)((data[2] << 8) | data[3]);
    int16_t raw_az = (int16_t)((data[4] << 8) | data[5]);

    // Convert to g units
    ax = raw_ax * ACCEL_SCALE;
    ay = raw_ay * ACCEL_SCALE;
    az = raw_az * ACCEL_SCALE;

    return true;
  }

  // ========================================================================
  // Sample once: read sensor, apply high-pass, store in FFT buffer
  // ========================================================================
  void sample_once_() {
    float ax, ay, az;

    if (!read_accel_g_(ax, ay, az)) {
      return;
    }

    // Compute vector magnitude
    float a_total = sqrt(ax*ax + ay*ay + az*az);

    // High-pass filter (remove DC component / gravity)
    dc_avg_ = dc_avg_ + DC_ALPHA * (a_total - dc_avg_);
    float a_hp = a_total - dc_avg_;

    // Store in FFT buffer
    vReal_[sample_index_] = a_hp;
    vImag_[sample_index_] = 0.0;
    sample_index_++;

    // Process window when full
    if (sample_index_ >= FFT_SAMPLES) {
      process_window_();

      // Handle overlap (50%)
      if (WINDOW_SHIFT < FFT_SAMPLES) {
        uint16_t keep = FFT_SAMPLES - WINDOW_SHIFT;
        for (uint16_t i = 0; i < keep; i++) {
          vReal_[i] = vReal_[i + WINDOW_SHIFT];
          vImag_[i] = 0.0;
        }
        sample_index_ = keep;
      } else {
        sample_index_ = 0;
      }
    }
  }

  // ========================================================================
  // Process full FFT window: compute RMS, FFT, bands, publish results
  // ========================================================================
  void process_window_() {
    // 1. Compute time-domain RMS
    double sum_sq = 0.0;
    for (uint16_t i = 0; i < FFT_SAMPLES; i++) {
      sum_sq += vReal_[i] * vReal_[i];
    }
    float rms = sqrt(sum_sq / FFT_SAMPLES);

    // Publish RMS
    if (rms_sensor_) {
      rms_sensor_->publish_state(rms);
    }

    // 2. Perform FFT (arduinoFFT v2 API)
    ArduinoFFT<double> FFT = ArduinoFFT<double>(vReal_, vImag_, FFT_SAMPLES, SAMPLE_FREQUENCY);
    FFT.windowing(FFTWindow::Hamming, FFTDirection::Forward);
    FFT.compute(FFTDirection::Forward);
    FFT.complexToMagnitude();

    // 3. Compute band energies
    float bands[FFT_BANDS];
    float bin_hz = SAMPLE_FREQUENCY / FFT_SAMPLES;
    float f_max = SAMPLE_FREQUENCY / 2.0f;
    float band_width = f_max / FFT_BANDS;
    uint16_t nyquist = FFT_SAMPLES / 2;

    for (uint8_t b = 0; b < FFT_BANDS; b++) {
      float f_start = b * band_width;
      float f_end = (b + 1) * band_width;

      uint16_t k_start = (uint16_t)(f_start / bin_hz);
      uint16_t k_end = (uint16_t)(f_end / bin_hz + 0.5f);

      // Clip range, ignore DC bin
      if (k_start < 1) k_start = 1;
      if (k_end >= nyquist) k_end = nyquist - 1;

      // Sum squared magnitudes in band
      double energy = 0.0;
      for (uint16_t k = k_start; k <= k_end; k++) {
        energy += vReal_[k] * vReal_[k];
      }
      bands[b] = energy;
    }

    // 4. Find dominant frequency (peak)
    double max_mag = 0.0;
    uint16_t max_k = 0;
    for (uint16_t k = 1; k < nyquist; k++) {
      if (vReal_[k] > max_mag) {
        max_mag = vReal_[k];
        max_k = k;
      }
    }
    float peak_freq = max_k * bin_hz;

    // 5. Build JSON string
    String json = "{";
    json += "\"fs\":" + String(SAMPLE_FREQUENCY, 1) + ",";
    json += "\"n\":" + String(FFT_SAMPLES) + ",";
    json += "\"bin_hz\":" + String(bin_hz, 3) + ",";
    json += "\"rms\":" + String(rms, 6) + ",";
    json += "\"peak_hz\":" + String(peak_freq, 2) + ",";
    json += "\"bands\":[";

    for (uint8_t b = 0; b < FFT_BANDS; b++) {
      if (b > 0) json += ",";
      json += String(bands[b], 2);
    }

    json += "]}";

    // Publish JSON spectrum
    if (spectrum_text_) {
      spectrum_text_->publish_state(json.c_str());
    }
  }
};

}  // namespace mpu_fft_json
