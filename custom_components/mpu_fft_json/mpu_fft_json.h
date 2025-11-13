#pragma once

#include "esphome.h"
#include "arduinoFFT.h"
#include <cmath>

using namespace esphome;

// Defaults and constants
static const float DC_ALPHA_DEFAULT = 0.01f;
static const uint32_t LOAD_WINDOW_US_DEFAULT = 1000000;  // 1s

static const uint8_t MPU_PWR_MGMT_1 = 0x6B;
static const uint8_t MPU_ACCEL_XOUT_H = 0x3B;
static const float ACCEL_SCALE = 1.0f / 16384.0f;  // g per LSB (±2g)
static const uint16_t MIN_FFT_SIZE = 64;           // minimum FFT length
static const uint16_t MAX_FFT_SIZE = 4096;         // safety upper bound

namespace mpu_fft_json {

class MPUFftJsonComponent : public Component, public i2c::I2CDevice {
 public:
  // Bindings
  void set_rms_sensor(sensor::Sensor *s) { rms_sensor_ = s; }
  void set_cpu_load_sensor(sensor::Sensor *s) { cpu_load_sensor_ = s; }
  void set_spectrum_text_sensor(text_sensor::TextSensor *s) { spectrum_text_ = s; }

  // Config
  void set_max_analysis_hz(float v) { max_analysis_hz_ = v; }
  void set_sample_frequency(float v) { sample_frequency_ = v; }
  void set_fft_samples(uint16_t v) { fft_samples_ = v; }
  void set_fft_bands(uint8_t v) { fft_bands_ = v; }
  void set_window_shift(uint16_t v) { window_shift_ = v; }
  void set_dc_alpha(float v) { dc_alpha_ = v; }
  void set_load_window_us(uint32_t v) { load_window_us_ = v; }

  // Diagnostics
  void set_bin_hz_sensor(sensor::Sensor *s) { bin_hz_sensor_ = s; }
  void set_fs_sensor(sensor::Sensor *s) { fs_sensor_ = s; }
  void set_fft_samples_sensor(sensor::Sensor *s) { fft_samples_sensor_ = s; }
  void set_fft_bands_sensor(sensor::Sensor *s) { fft_bands_sensor_ = s; }
  void set_max_analysis_hz_sensor(sensor::Sensor *s) { max_analysis_hz_sensor_ = s; }

  ~MPUFftJsonComponent() { delete[] vReal_; delete[] vImag_; }

  void setup() override {
    if (!this->write_byte(MPU_PWR_MGMT_1, 0x00)) {
      ESP_LOGE(TAG, "MPU6050 wake-up failed");
      this->mark_failed();
      return;
    }
    esphome::delay(100);

    // Normalize configuration
    if (sample_frequency_ < 10.0f) sample_frequency_ = 10.0f;
    if (sample_frequency_ > 5000.0f) sample_frequency_ = 5000.0f;
    if (!is_power_of_two(fft_samples_)) fft_samples_ = next_power_of_two(fft_samples_);
    if (fft_samples_ < MIN_FFT_SIZE) fft_samples_ = MIN_FFT_SIZE;
    if (fft_samples_ > MAX_FFT_SIZE) fft_samples_ = MAX_FFT_SIZE;
    if (fft_bands_ == 0) fft_bands_ = 1; if (fft_bands_ > 64) fft_bands_ = 64;
    if (window_shift_ == 0 || window_shift_ >= fft_samples_) window_shift_ = fft_samples_ / 2; // 50% overlap

    sample_period_us_ = (uint32_t)(1000000.0f / sample_frequency_);

    vReal_ = new double[fft_samples_];
    vImag_ = new double[fft_samples_];
    for (uint16_t i = 0; i < fft_samples_; i++) { vReal_[i] = 0.0; vImag_[i] = 0.0; }

    last_sample_us_ = esphome::micros();
    load_window_start_us_ = last_sample_us_;
    sample_index_ = 0; dc_avg_ = 1.0f; busy_time_us_ = 0;
    ESP_LOGI(TAG, "Configured fs=%.1fHz n=%u bands=%u overlap=%u max_hz=%.1f", sample_frequency_, fft_samples_, fft_bands_, window_shift_, max_analysis_hz_);
  }

  void loop() override {
    uint32_t loop_start = esphome::micros();
    uint32_t now = esphome::micros();

    if ((now - last_sample_us_) >= sample_period_us_) {
      last_sample_us_ = now;
      sample_once_();
    }

    uint32_t window_elapsed = now - load_window_start_us_;
    if (window_elapsed >= load_window_us_) {
      float load = (float)busy_time_us_ / (float)window_elapsed;
      float cpu = load * 100.0f;
      if (cpu_load_sensor_ && (cpu_load_sensor_->has_state() || cpu > 1.0f))
        cpu_load_sensor_->publish_state(cpu);
      busy_time_us_ = 0;
      load_window_start_us_ = now;
    }

    uint32_t loop_end = esphome::micros();
    busy_time_us_ += (loop_end - loop_start);
  }

protected:
  // Helpers
  static bool is_power_of_two(uint32_t v) { return v && ((v & (v - 1)) == 0); }
  static uint32_t next_power_of_two(uint32_t v) { if (v == 0) return 1; v--; v |= v >> 1; v |= v >> 2; v |= v >> 4; v |= v >> 8; v |= v >> 16; return v + 1; }
  static constexpr const char *TAG = "MPU_FFT";
  // Sensors
  sensor::Sensor *rms_sensor_{nullptr};
  sensor::Sensor *cpu_load_sensor_{nullptr};
  text_sensor::TextSensor *spectrum_text_{nullptr};
  // Diagnostics
  sensor::Sensor *bin_hz_sensor_{nullptr};
  sensor::Sensor *fs_sensor_{nullptr};
  sensor::Sensor *fft_samples_sensor_{nullptr};
  sensor::Sensor *fft_bands_sensor_{nullptr};
  sensor::Sensor *max_analysis_hz_sensor_{nullptr};

  // Buffers
  double *vReal_{nullptr};
  double *vImag_{nullptr};

  // Sampling
  uint16_t sample_index_{0};
  uint32_t last_sample_us_{0};
  float dc_avg_{1.0f};

  // CPU load
  uint32_t load_window_start_us_{0};
  uint32_t busy_time_us_{0};

  // Config
  float sample_frequency_{1000.0f};
  uint16_t fft_samples_{512};
  uint8_t fft_bands_{16};
  uint16_t window_shift_{0};
  float dc_alpha_{DC_ALPHA_DEFAULT};
  uint32_t load_window_us_{LOAD_WINDOW_US_DEFAULT};
  uint32_t sample_period_us_{(uint32_t)(1000000.0f / 1000.0f)};
  float max_analysis_hz_{300.0f};

  bool read_accel_g_(float &ax, float &ay, float &az) {
    uint8_t data[6];
    if (!this->read_bytes(MPU_ACCEL_XOUT_H, data, 6)) return false;
    int16_t raw_ax = (int16_t)((data[0] << 8) | data[1]);
    int16_t raw_ay = (int16_t)((data[2] << 8) | data[3]);
    int16_t raw_az = (int16_t)((data[4] << 8) | data[5]);
    ax = raw_ax * ACCEL_SCALE;
    ay = raw_ay * ACCEL_SCALE;
    az = raw_az * ACCEL_SCALE;
    return true;
  }

  void sample_once_() {
    float ax, ay, az;
    if (!read_accel_g_(ax, ay, az)) return;

    float a_total = sqrt(ax * ax + ay * ay + az * az);
    dc_avg_ = dc_avg_ + dc_alpha_ * (a_total - dc_avg_);
    float a_hp = a_total - dc_avg_;

    vReal_[sample_index_] = a_hp;
    vImag_[sample_index_] = 0.0;
    sample_index_++;

    if (sample_index_ >= fft_samples_) {
      process_window_();

      uint16_t shift = (window_shift_ > 0 && window_shift_ < fft_samples_) ? window_shift_ : (fft_samples_ / 2);
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

  void process_window_() {
    // RMS
    double sum_sq = 0.0;
    for (uint16_t i = 0; i < fft_samples_; i++) sum_sq += vReal_[i] * vReal_[i];
    float rms = sqrt(sum_sq / fft_samples_);
    if (rms_sensor_) rms_sensor_->publish_state(rms);

    // FFT
    ArduinoFFT<double> FFT(vReal_, vImag_, fft_samples_, sample_frequency_);
    FFT.windowing(FFTWindow::Hamming, FFTDirection::Forward);
    FFT.compute(FFTDirection::Forward);
    FFT.complexToMagnitude();

    float bin_hz = sample_frequency_ / fft_samples_;
    float f_nyquist = sample_frequency_ / 2.0f;
    float f_max = (max_analysis_hz_ > 0.0f && max_analysis_hz_ < f_nyquist) ? max_analysis_hz_ : f_nyquist;
    float band_width = f_max / (float)fft_bands_;
    uint16_t nyquist = fft_samples_ / 2;

    // Peak
    double max_mag = 0.0;
    uint16_t max_k = 0;
    for (uint16_t k = 1; k < nyquist; k++) if (vReal_[k] > max_mag) { max_mag = vReal_[k]; max_k = k; }
    float peak_freq = max_k * bin_hz;

    // Diagnostics
    if (bin_hz_sensor_) bin_hz_sensor_->publish_state(bin_hz);
    if (fs_sensor_) fs_sensor_->publish_state(sample_frequency_);
    if (fft_samples_sensor_) fft_samples_sensor_->publish_state((float)fft_samples_);
    if (fft_bands_sensor_) fft_bands_sensor_->publish_state((float)fft_bands_);
    if (max_analysis_hz_sensor_) max_analysis_hz_sensor_->publish_state(f_max);

    // JSON
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
      for (uint16_t k = k_start; k <= k_end; k++) energy += vReal_[k] * vReal_[k];
      if (b > 0) json += ",";
      json += String(energy, 2);
    }

    json += "],";

    json += "\"band_center\":[";
    for (uint8_t b = 0; b < fft_bands_; b++) { if (b > 0) json += ","; json += String((b + 0.5f) * band_width, 1); }
    json += "],";

    json += "\"band_low\":[";
    for (uint8_t b = 0; b < fft_bands_; b++) { if (b > 0) json += ","; json += String(b * band_width, 1); }
    json += "],";

    json += "\"band_high\":[";
    for (uint8_t b = 0; b < fft_bands_; b++) { if (b > 0) json += ","; json += String((b + 1) * band_width, 1); }
    json += "]}";

    if (spectrum_text_) spectrum_text_->publish_state(json.c_str());
  }
};

}  // namespace mpu_fft_json
