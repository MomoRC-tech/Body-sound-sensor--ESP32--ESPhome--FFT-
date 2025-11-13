"""MPU FFT JSON Component for ESPHome."""
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import i2c
from esphome.components import time as time_comp
from esphome.const import CONF_ID

CODEOWNERS = ["@MomoRC-tech"]
DEPENDENCIES = ["i2c"]
AUTO_LOAD = ["sensor", "text_sensor"]

CONF_MPU_FFT_JSON_ID = "mpu_fft_json_id"
CONF_MAX_ANALYSIS_HZ = "max_analysis_hz"
CONF_SAMPLE_FREQUENCY = "sample_frequency"
CONF_FFT_SAMPLES = "fft_samples"
CONF_FFT_BANDS = "fft_bands"
CONF_WINDOW_SHIFT = "window_shift"
CONF_DC_ALPHA = "dc_alpha"
CONF_LOAD_WINDOW_US = "load_window_us"
CONF_TIME_ID = "time_id"

mpu_fft_json_ns = cg.esphome_ns.namespace("mpu_fft_json")
MPUFftJsonComponent = mpu_fft_json_ns.class_(
    "MPUFftJsonComponent", cg.Component, i2c.I2CDevice
)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(MPUFftJsonComponent),
            cv.Optional(CONF_MAX_ANALYSIS_HZ, default=300.0): cv.float_,
            cv.Optional(CONF_SAMPLE_FREQUENCY, default=1000.0): cv.float_,
            cv.Optional(CONF_FFT_SAMPLES, default=512): cv.one_of(128, 256, 512, 1024, 2048, int=True),
            cv.Optional(CONF_FFT_BANDS, default=16): cv.int_range(min=1, max=64),
            cv.Optional(CONF_WINDOW_SHIFT, default=0): cv.int_range(min=0, max=4096),
            cv.Optional(CONF_DC_ALPHA, default=0.01): cv.float_,
            cv.Optional(CONF_LOAD_WINDOW_US, default=1000000): cv.positive_int,
            cv.Optional(CONF_TIME_ID): cv.use_id(time_comp.RealTimeClock),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(i2c.i2c_device_schema(0x68))
)


async def to_code(config):
    """Code generation for MPU FFT JSON component."""
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)

    # Apply optional configuration
    if CONF_MAX_ANALYSIS_HZ in config:
        cg.add(var.set_max_analysis_hz(config[CONF_MAX_ANALYSIS_HZ]))
    if CONF_SAMPLE_FREQUENCY in config:
        cg.add(var.set_sample_frequency(config[CONF_SAMPLE_FREQUENCY]))
    if CONF_FFT_SAMPLES in config:
        cg.add(var.set_fft_samples(config[CONF_FFT_SAMPLES]))
    if CONF_FFT_BANDS in config:
        cg.add(var.set_fft_bands(config[CONF_FFT_BANDS]))
    if CONF_WINDOW_SHIFT in config:
        cg.add(var.set_window_shift(config[CONF_WINDOW_SHIFT]))
    if CONF_DC_ALPHA in config:
        cg.add(var.set_dc_alpha(config[CONF_DC_ALPHA]))
    if CONF_LOAD_WINDOW_US in config:
        cg.add(var.set_load_window_us(config[CONF_LOAD_WINDOW_US]))

    # Add the library dependency
    cg.add_library("arduinoFFT", "^2.0.4")

    # Optional time component for wall-clock epoch
    if CONF_TIME_ID in config:
        time_var = await cg.get_variable(config[CONF_TIME_ID])
        cg.add(var.set_time(time_var))
