"""Sensor platform for MPU FFT JSON component."""
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import (
    CONF_ID,
    DEVICE_CLASS_EMPTY,
    STATE_CLASS_MEASUREMENT,
    UNIT_PERCENT,
)
from . import MPUFftJsonComponent, CONF_MPU_FFT_JSON_ID, mpu_fft_json_ns

CONF_RMS = "rms"
CONF_CPU_LOAD = "cpu_load"
CONF_BIN_HZ = "bin_hz"
CONF_FS = "sample_frequency"
CONF_FFT_SAMPLES = "fft_samples"
CONF_FFT_BANDS = "fft_bands"
CONF_MAX_ANALYSIS_HZ = "max_analysis_hz"
CONF_WINDOW_SHIFT_DIAG = "window_shift"

# Custom unit for acceleration
UNIT_G = "g"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_MPU_FFT_JSON_ID): cv.use_id(MPUFftJsonComponent),
        cv.Optional(CONF_RMS): sensor.sensor_schema(
            unit_of_measurement=UNIT_G,
            accuracy_decimals=4,
            device_class=DEVICE_CLASS_EMPTY,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_CPU_LOAD): sensor.sensor_schema(
            unit_of_measurement=UNIT_PERCENT,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_EMPTY,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_BIN_HZ): sensor.sensor_schema(
            unit_of_measurement="Hz/bin",
            accuracy_decimals=3,
            device_class=DEVICE_CLASS_EMPTY,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_FS): sensor.sensor_schema(
            unit_of_measurement="Hz",
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_EMPTY,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_FFT_SAMPLES): sensor.sensor_schema(
            unit_of_measurement="samples",
            accuracy_decimals=0,
            device_class=DEVICE_CLASS_EMPTY,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_FFT_BANDS): sensor.sensor_schema(
            accuracy_decimals=0,
            device_class=DEVICE_CLASS_EMPTY,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_MAX_ANALYSIS_HZ): sensor.sensor_schema(
            unit_of_measurement="Hz",
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_EMPTY,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_WINDOW_SHIFT_DIAG): sensor.sensor_schema(
            unit_of_measurement="samples",
            accuracy_decimals=0,
            device_class=DEVICE_CLASS_EMPTY,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
    }
)


async def to_code(config):
    """Code generation for MPU FFT JSON sensors."""
    parent = await cg.get_variable(config[CONF_MPU_FFT_JSON_ID])

    if rms_config := config.get(CONF_RMS):
        sens = await sensor.new_sensor(rms_config)
        cg.add(parent.set_rms_sensor(sens))

    if cpu_load_config := config.get(CONF_CPU_LOAD):
        sens = await sensor.new_sensor(cpu_load_config)
        cg.add(parent.set_cpu_load_sensor(sens))

    if bin_hz_config := config.get(CONF_BIN_HZ):
        sens = await sensor.new_sensor(bin_hz_config)
        cg.add(parent.set_bin_hz_sensor(sens))

    if fs_config := config.get(CONF_FS):
        sens = await sensor.new_sensor(fs_config)
        cg.add(parent.set_fs_sensor(sens))

    if n_config := config.get(CONF_FFT_SAMPLES):
        sens = await sensor.new_sensor(n_config)
        cg.add(parent.set_fft_samples_sensor(sens))

    if bands_config := config.get(CONF_FFT_BANDS):
        sens = await sensor.new_sensor(bands_config)
        cg.add(parent.set_fft_bands_sensor(sens))

    if max_config := config.get(CONF_MAX_ANALYSIS_HZ):
        sens = await sensor.new_sensor(max_config)
        cg.add(parent.set_max_analysis_hz_sensor(sens))

    if ws_config := config.get(CONF_WINDOW_SHIFT_DIAG):
        sens = await sensor.new_sensor(ws_config)
        cg.add(parent.set_window_shift_sensor(sens))
