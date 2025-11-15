#pragma once

#include "esphome.h"
#if defined(__has_include)
#  if __has_include("esp_dsp.h")
#    define MPUFFT_USE_ESPDSP 1
#  else
#    define MPUFFT_USE_ESPDSP 0
#  endif
#else
#  define MPUFFT_USE_ESPDSP 0
#endif

#if MPUFFT_USE_ESPDSP
#include "esp_dsp.h"
#else
#include "arduinoFFT.h"
#endif
#include "esphome/components/time/real_time_clock.h"
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
  void set_time(time::RealTimeClock *t) { time_ = t; }

  // Diagnostics
  void set_bin_hz_sensor(sensor::Sensor *s) { bin_hz_sensor_ = s; }
  void set_fs_sensor(sensor::Sensor *s) { fs_sensor_ = s; }
  void set_fft_samples_sensor(sensor::Sensor *s) { fft_samples_sensor_ = s; }
  void set_fft_bands_sensor(sensor::Sensor *s) { fft_bands_sensor_ = s; }
  void set_max_analysis_hz_sensor(sensor::Sensor *s) { max_analysis_hz_sensor_ = s; }
  void set_window_shift_sensor(sensor::Sensor *s) { window_shift_sensor_ = s; }

  ~MPUFftJsonComponent() {
    delete[] vReal_;
    delete[] vImag_;
    delete[] fft_real_;
    delete[] fft_imag_;
#if MPUFFT_USE_ESPDSP
    delete[] fft_work_fc32_;
    delete[] fft_mag_f32_;
    delete[] window_f32_;
#endif
  }

  void setup() override {
    if (!this->write_byte(MPU_PWR_MGMT_1, 0x00)) {
      ESP_LOGE(TAG, "MPU6050 wake-up failed");
      this->mark_failed();
      return;
    }
    esphome::delay(100);

    // Normalize configuration
    if (sample_frequency_ < 10.0f) {
      sample_frequency_ = 10.0f;
    }
    if (sample_frequency_ > 5000.0f) {
      sample_frequency_ = 5000.0f;
    }
    if (!is_power_of_two(fft_samples_)) {
      fft_samples_ = next_power_of_two(fft_samples_);
    }
    if (fft_samples_ < MIN_FFT_SIZE) {
      fft_samples_ = MIN_FFT_SIZE;
    }
    if (fft_samples_ > MAX_FFT_SIZE) {
      fft_samples_ = MAX_FFT_SIZE;
    }
    if (fft_bands_ == 0) {
      fft_bands_ = 1;
    }
    if (fft_bands_ > 64) {
      fft_bands_ = 64;
    }
    if (window_shift_ == 0 || window_shift_ >= fft_samples_) {
      // 50% overlap
      window_shift_ = fft_samples_ / 2;
    }

    sample_period_us_ = (uint32_t)(1000000.0f / sample_frequency_);

    vReal_ = new double[fft_samples_];
    vImag_ = new double[fft_samples_];
    for (uint16_t i = 0; i < fft_samples_; i++) {
      vReal_[i] = 0.0;
      vImag_[i] = 0.0;
    }

    fft_real_ = new double[fft_samples_];
    fft_imag_ = new double[fft_samples_];
    #if MPUFFT_USE_ESPDSP
        // Initialize ESP-DSP FFT tables for current size
        // 4 is maximum stages with 16-bit sin/cos table depth; NULL means internal static table
        dsps_fft2r_init_fc32(nullptr, fft_samples_);
        // Allocate working buffers: interleaved complex array (2*N) and magnitude (N)
        fft_work_fc32_ = new float[fft_samples_ * 2];
        fft_mag_f32_ = new float[fft_samples_];
        // Precompute Hamming window
        window_f32_ = new float[fft_samples_];
        for (uint16_t i = 0; i < fft_samples_; i++) {
          window_f32_[i] = 0.54f - 0.46f * cosf(2.0f * (float)M_PI * (float)i / (float)(fft_samples_ - 1));
        }
    #endif
    for (uint16_t i = 0; i < fft_samples_; i++) {
      fft_real_[i] = 0.0;
      fft_imag_[i] = 0.0;
    }

    last_sample_us_ = esphome::micros();
    load_window_start_us_ = last_sample_us_;
    sample_index_ = 0;
    dc_ax_ = dc_ay_ = dc_az_ = 0.0f;
    axis_dc_init_ = false;
    busy_time_us_ = 0;
    ESP_LOGI(
        TAG,
        "Configured fs=%.1fHz n=%u bands=%u overlap=%u max_hz=%.1f",
        sample_frequency_,
        fft_samples_,
        fft_bands_,
        window_shift_,
        max_analysis_hz_);
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
  static uint32_t next_power_of_two(uint32_t v) {
    if (v == 0) {
      return 1;
    }
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    return v + 1;
  }
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
  sensor::Sensor *window_shift_sensor_{nullptr};

  // Buffers
  double *vReal_{nullptr};
  double *vImag_{nullptr};
  double *fft_real_{nullptr};
  double *fft_imag_{nullptr};
#if MPUFFT_USE_ESPDSP
  // ESP-DSP working buffers
  float *fft_work_fc32_{nullptr};  // interleaved complex: [Re0, Im0, Re1, Im1, ...]
  float *fft_mag_f32_{nullptr};    // magnitude spectrum
  float *window_f32_{nullptr};     // Hamming window
#endif

  // Sampling
  uint16_t sample_index_{0};
  uint32_t last_sample_us_{0};
  // Per-axis DC averages for high-pass filtering
  float dc_ax_{0.0f};
  float dc_ay_{0.0f};
  float dc_az_{0.0f};
  bool axis_dc_init_{false};

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
  uint32_t seq_{0};
  time::RealTimeClock *time_{nullptr};
  uint64_t epoch_base_ms_{0};
  bool epoch_base_set_{false};

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
    float ax;
    float ay;
    float az;
    if (!read_accel_g_(ax, ay, az)) {
      return;
    }

    // Initialize DC baselines on first sample
    if (!axis_dc_init_) {
      dc_ax_ = ax;
      dc_ay_ = ay;
      dc_az_ = az;
      axis_dc_init_ = true;
    }
    // Update per-axis DC averages
    dc_ax_ += dc_alpha_ * (ax - dc_ax_);
    dc_ay_ += dc_alpha_ * (ay - dc_ay_);
    dc_az_ += dc_alpha_ * (az - dc_az_);
    float ax_hp = ax - dc_ax_;
    float ay_hp = ay - dc_ay_;
    float az_hp = az - dc_az_;
    float a_hp = sqrt(ax_hp * ax_hp + ay_hp * ay_hp + az_hp * az_hp);

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
    for (uint16_t i = 0; i < fft_samples_; i++) {
      sum_sq += vReal_[i] * vReal_[i];
    }
    float rms = sqrt(sum_sq / fft_samples_);
    if (rms_sensor_) {
      rms_sensor_->publish_state(rms);
    }
    // Min/max of high-passed magnitude for diagnostics
    double hp_min = vReal_[0];
    double hp_max = vReal_[0];
    for (uint16_t i = 1; i < fft_samples_; i++) {
      if (vReal_[i] < hp_min) hp_min = vReal_[i];
      if (vReal_[i] > hp_max) hp_max = vReal_[i];
    }

    // FFT
#if MPUFFT_USE_ESPDSP
    // Load windowed samples into interleaved complex buffer
    for (uint16_t i = 0; i < fft_samples_; i++) {
      float s = (float)vReal_[i];
      s *= window_f32_[i];
      fft_work_fc32_[2 * i + 0] = s;   // real
      fft_work_fc32_[2 * i + 1] = 0.0f; // imag
    }
    // In-place complex FFT
    dsps_fft2r_fc32(fft_work_fc32_, fft_samples_);
    dsps_bit_rev2r_fc32(fft_work_fc32_, fft_samples_);
    // Convert complex output to magnitude spectrum
    dsps_cplx2reC_fc32(fft_work_fc32_, fft_mag_f32_, fft_samples_);
#else
    for (uint16_t i = 0; i < fft_samples_; i++) {
      fft_real_[i] = vReal_[i];
      fft_imag_[i] = 0.0;
    }
    ArduinoFFT<double> FFT(fft_real_, fft_imag_, fft_samples_, sample_frequency_);
    FFT.windowing(FFTWindow::Hamming, FFTDirection::Forward);
    FFT.compute(FFTDirection::Forward);
    FFT.complexToMagnitude();
#endif

    float bin_hz = sample_frequency_ / fft_samples_;
    float f_nyquist = sample_frequency_ / 2.0f;
    float f_max = (max_analysis_hz_ > 0.0f && max_analysis_hz_ < f_nyquist) ? max_analysis_hz_ : f_nyquist;
    float band_width = f_max / (float)fft_bands_;
    uint16_t nyquist = fft_samples_ / 2;

    // Peak
    double max_mag = 0.0;
    uint16_t max_k = 0;
#if MPUFFT_USE_ESPDSP
    for (uint16_t k = 1; k < nyquist; k++) {
      double m = (double)fft_mag_f32_[k];
      if (m > max_mag) {
        max_mag = m;
        max_k = k;
      }
    }
#else
    for (uint16_t k = 1; k < nyquist; k++) {
      if (fft_real_[k] > max_mag) {
        max_mag = fft_real_[k];
        max_k = k;
      }
    }
#endif
    float peak_freq = max_k * bin_hz;

    // Diagnostics
    if (bin_hz_sensor_) {
      bin_hz_sensor_->publish_state(bin_hz);
    }
    if (fs_sensor_) {
      fs_sensor_->publish_state(sample_frequency_);
    }
    if (fft_samples_sensor_) {
      fft_samples_sensor_->publish_state((float)fft_samples_);
    }
    if (fft_bands_sensor_) {
      fft_bands_sensor_->publish_state((float)fft_bands_);
    }
    if (max_analysis_hz_sensor_) {
      max_analysis_hz_sensor_->publish_state(f_max);
    }
    if (window_shift_sensor_) {
      window_shift_sensor_->publish_state((float)window_shift_);
    }

    // JSON
    // Timing metadata derived from sampling schedule
    uint32_t win_us = (uint32_t)fft_samples_ * sample_period_us_;
    uint32_t hop_us = (uint32_t)window_shift_ * sample_period_us_;
    uint32_t end_us = last_sample_us_;
    uint32_t center_us = end_us - (win_us / 2u);
    uint32_t ts_ms = center_us / 1000u;  // monotonic since boot

    // Establish epoch base when time becomes valid (once)
    if (!epoch_base_set_ && time_ != nullptr) {
      auto now = time_->now();
      if (now.is_valid()) {
        uint64_t now_ms = (uint64_t)now.timestamp * 1000ULL;
        uint64_t mono_ms = (uint64_t)esphome::millis();
        epoch_base_ms_ = now_ms - mono_ms;
        epoch_base_set_ = true;
      }
    }
    uint64_t epoch_ms = epoch_base_set_ ? (epoch_base_ms_ + (uint64_t)ts_ms) : 0ULL;

    String json = "{";
    json += "\"schema_version\":1,";  // public JSON contract version (increment on breaking changes)
    json += "\"fs\":" + String(sample_frequency_, 1) + ",";
    json += "\"n\":" + String(fft_samples_) + ",";
    json += "\"bin_hz\":" + String(bin_hz, 3) + ",";
    json += "\"rms\":" + String(rms, 6) + ",";
    json += "\"peak_hz\":" + String(peak_freq, 2) + ",";
    json += "\"max_analysis_hz\":" + String(f_max, 1) + ",";
    json += "\"ts_ms\":" + String(ts_ms) + ",";
    json += "\"win_ms\":" + String((double)win_us / 1000.0, 1) + ",";
    json += "\"hop_ms\":" + String((double)hop_us / 1000.0, 1) + ",";
    json += "\"seq\":" + String(seq_) + ",";
    if (epoch_base_set_) {
      json += "\"epoch_ms\":" + String((double)epoch_ms, 0) + ",";
    }
    json += "\"bands\":[]";  // placeholder replace next block

    // Build bands array
    String bands = "";
    for (uint8_t b = 0; b < fft_bands_; b++) {
      float f_start = b * band_width;
      float f_end = (b + 1) * band_width;
      uint16_t k_start = (uint16_t)(f_start / bin_hz);
      uint16_t k_end = (uint16_t)(f_end / bin_hz + 0.5f);
      if (k_start < 1) {
        k_start = 1;
      }
      if (k_end >= nyquist) {
        k_end = nyquist - 1;
      }
            double energy = 0.0;
            for (uint16_t k = k_start; k <= k_end; k++) {
      #if MPUFFT_USE_ESPDSP
        double m = (double)fft_mag_f32_[k];
        energy += m * m;
      #else
        energy += fft_real_[k] * fft_real_[k];
      #endif
            }
      if (b > 0) {
        bands += ",";
      }
      bands += String(energy, 6);  // higher precision to expose small energies
    }
    json.replace("\"bands\":[]", "\"bands\":[" + bands + "]");

    // band_center
    String band_center = "";
    for (uint8_t b = 0; b < fft_bands_; b++) {
      if (b > 0) {
        band_center += ",";
      }
      band_center += String((b + 0.5f) * band_width, 1);
    }
    json += ",\"band_center\":[" + band_center + "]";

    // band_low
    String band_low = "";
    for (uint8_t b = 0; b < fft_bands_; b++) {
      if (b > 0) {
        band_low += ",";
      }
      band_low += String(b * band_width, 1);
    }
    json += ",\"band_low\":[" + band_low + "]";

    // band_high
    String band_high = "";
    for (uint8_t b = 0; b < fft_bands_; b++) {
      if (b > 0) {
        band_high += ",";
      }
      band_high += String((b + 1) * band_width, 1);
    }
    json += ",\"band_high\":[" + band_high + "]";
    json += "}";

    if (spectrum_text_) {
      spectrum_text_->publish_state(json.c_str());
    }
    ESP_LOGD(TAG, "seq=%u rms=%.6f hp_min=%.6f hp_max=%.6f peak=%.2fHz", seq_, rms, hp_min, hp_max, peak_freq);

    // advance sequence counter
    seq_++;
  }
};

}  // namespace mpu_fft_json
