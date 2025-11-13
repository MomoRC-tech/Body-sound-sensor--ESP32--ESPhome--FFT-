#pragma once

#include "esphome.h"
#include "arduinoFFT.h"
#include <cmath>

using namespace esphome;

// ============================================================================
// CONFIGURATION CONSTANTS (Tunable Parameters)
// ============================================================================

// Sampling configuration (defaults; can be overridden via YAML)
// Values are stored in member variables; these are only defaults for docs context.

// FFT configuration (defaults; can be overridden via YAML)

// High-pass filter coefficient for DC removal
// High-pass filter coefficient for DC removal (configurable)
static const float    DC_ALPHA_DEFAULT  = 0.01f;

// CPU load measurement window (1 second)
// CPU load measurement window default (configurable)
static const uint32_t LOAD_WINDOW_US_DEFAULT = 1000000;

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
  void set_max_analysis_hz(float hz) { max_analysis_hz_ = hz; }
  void set_sample_frequency(float fs) { sample_frequency_ = fs; }
  void set_fft_samples(uint16_t n) { fft_samples_ = n; }
  void set_fft_bands(uint8_t b) { fft_bands_ = b; }
  void set_window_shift(uint16_t s) { window_shift_ = s; }
  void set_dc_alpha(float a) { dc_alpha_ = a; }
  void set_load_window_us(uint32_t us) { load_window_us_ = us; }

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

    // Apply derived parameters and allocate buffers
    if (fft_samples_ < 64) fft_samples_ = 64; // basic sanity
    sample_period_us_ = (uint32_t)(1000000.0f / sample_frequency_);

    // Allocate FFT buffers
    vReal_ = new double[fft_samples_];
    vImag_ = new double[fft_samples_];

    // Initialize timing and state
    last_sample_us_ = esphome::micros();
    sample_index_ = 0;
    dc_avg_ = 1.0f;  // Initialize to ~1g (gravity)

    // Initialize CPU load tracking
    load_window_start_us_ = last_sample_us_;
    busy_time_us_ = 0;

    // Clear buffers
    for (uint16_t i = 0; i < fft_samples_; i++) {
      vReal_[i] = 0.0;
      vImag_[i] = 0.0;
    }

    ESP_LOGI("MPU_FFT", "MPU FFT Component initialized successfully");
    ESP_LOGI("MPU_FFT", "Sample rate: %.0f Hz, FFT size: %d, Bands: %d",
         sample_frequency_, fft_samples_, fft_bands_);
  }

  void loop() override {
    uint32_t loop_start = esphome::micros();
    uint32_t now = esphome::micros();

    // Maintain constant sampling rate
    if ((now - last_sample_us_) >= sample_period_us_) {
      last_sample_us_ = now;
      sample_once_();
    }

    // Update CPU load periodically
    uint32_t window_elapsed = now - load_window_start_us_;
    if (window_elapsed >= load_window_us_) {
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
  double *vReal_{nullptr};
  double *vImag_{nullptr};

  // Sampling state
  uint16_t sample_index_ = 0;
  uint32_t last_sample_us_ = 0;
  float dc_avg_ = 1.0f;

  // CPU load tracking
  uint32_t load_window_start_us_ = 0;
  uint32_t busy_time_us_ = 0;

  // Configurable parameters (with defaults)
  float sample_frequency_ = 1000.0f;
  uint16_t fft_samples_ = 512;         // must be power of 2
  uint8_t fft_bands_ = 16;
  uint16_t window_shift_ = 0;          // 0 => use 50% overlap
  float dc_alpha_ = DC_ALPHA_DEFAULT;
  uint32_t load_window_us_ = LOAD_WINDOW_US_DEFAULT;
  uint32_t sample_period_us_ = (uint32_t)(1000000.0f / 1000.0f);

  // Configurable top analysis frequency (defaults to 300 Hz; clamped to Nyquist at runtime)
  float max_analysis_hz_ = 300.0f;

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
    dc_avg_ = dc_avg_ + dc_alpha_ * (a_total - dc_avg_);
    float a_hp = a_total - dc_avg_;

    // Store in FFT buffer
    vReal_[sample_index_] = a_hp;
    vImag_[sample_index_] = 0.0;
    sample_index_++;

    // Process window when full
    if (sample_index_ >= fft_samples_) {
      process_window_();

      // Handle overlap (50%)
      uint16_t shift = (window_shift_ > 0 && window_shift_ < fft_samples_)
                         ? window_shift_ : (fft_samples_ / 2);
      if (shift < fft_samples_) {
        uint16_t keep = fft_samples_ - shift;
        for (uint16_t i = 0; i < keep; i++) {
          vReal_[i] = vReal_[i + shift];
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
    for (uint16_t i = 0; i < fft_samples_; i++) {
      sum_sq += vReal_[i] * vReal_[i];
    }
    float rms = sqrt(sum_sq / fft_samples_);

    // Publish RMS
    if (rms_sensor_) {
      rms_sensor_->publish_state(rms);
    }

    // 2. Perform FFT (arduinoFFT v2 API)
    ArduinoFFT<double> FFT = ArduinoFFT<double>(vReal_, vImag_, fft_samples_, sample_frequency_);
    FFT.windowing(FFTWindow::Hamming, FFTDirection::Forward);
    FFT.compute(FFTDirection::Forward);
    FFT.complexToMagnitude();

    // 3. Compute band energies
    float bands[FFT_BANDS];
    float bin_hz = SAMPLE_FREQUENCY / FFT_SAMPLES;
    float f_nyquist = SAMPLE_FREQUENCY / 2.0f;
    float f_max = f_nyquist;
    if (max_analysis_hz_ > 0.0f && max_analysis_hz_ < f_nyquist) {
      f_max = max_analysis_hz_;
    }
    float band_width = f_max / FFT_BANDS;
    uint16_t nyquist = FFT_SAMPLES / 2;

    float bin_hz = sample_frequency_ / fft_samples_;
    float f_nyquist = sample_frequency_ / 2.0f;
    float f_max = f_nyquist;
      uint16_t k_start = (uint16_t)(f_start / bin_hz);
      uint16_t k_end = (uint16_t)(f_end / bin_hz + 0.5f);

    float band_width = f_max / (float)fft_bands_;
    uint16_t nyquist = fft_samples_ / 2;
      if (k_end >= nyquist) k_end = nyquist - 1;
    // Compute dominant frequency (peak) first
    double max_mag = 0.0;
    uint16_t max_k = 0;
    for (uint16_t k = 1; k < nyquist; k++) {
      if (vReal_[k] > max_mag) {
        max_mag = vReal_[k];
        max_k = k;
      }
    }
    float peak_freq = max_k * bin_hz;

    // Build JSON string and compute band energies on the fly
    String json = "{";
    json += "\"fs\":" + String(sample_frequency_, 1) + ",";
    json += "\"n\":" + String(fft_samples_) + ",";
    json += "\"bin_hz\":" + String(bin_hz, 3) + ",";
    json += "\"rms\":" + String(rms, 6) + ",";
    json += "\"peak_hz\":" + String(peak_freq, 2) + ",";
    json += "\"max_analysis_hz\":" + String(f_max, 1) + ",";
    json += "\"bands\":[";

    for (uint8_t b = 0; b < fft_bands_; b++) {
      float f_start = b * band_width;
      float f_end = (b + 1) * band_width;

      uint16_t k_start = (uint16_t)(f_start / bin_hz);
      uint16_t k_end = (uint16_t)(f_end / bin_hz + 0.5f);

      if (k_start < 1) k_start = 1;
      if (k_end >= nyquist) k_end = nyquist - 1;

      double energy = 0.0;
      for (uint16_t k = k_start; k <= k_end; k++) {
        energy += vReal_[k] * vReal_[k];
      }
      if (b > 0) json += ",";
      json += String(energy, 2);
    }

    json += "],";

    // Band metadata
    json += "\"band_center\":[";
    for (uint8_t b = 0; b < fft_bands_; b++) {
      if (b > 0) json += ",";
      float f_center = (b + 0.5f) * band_width;
      json += String(f_center, 1);
    }
    json += "],";

    json += "\"band_low\":[";
    for (uint8_t b = 0; b < fft_bands_; b++) {
      if (b > 0) json += ",";
      float f_low = b * band_width;
      json += String(f_low, 1);
    }
    json += "],";

    json += "\"band_high\":[";
    for (uint8_t b = 0; b < fft_bands_; b++) {
      if (b > 0) json += ",";
      float f_high = (b + 1) * band_width;
      json += String(f_high, 1);
    }
    json += "]}";
