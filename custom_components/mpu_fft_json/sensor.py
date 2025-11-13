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
