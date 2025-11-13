"""MPU FFT JSON Component for ESPHome."""
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import i2c
from esphome.const import CONF_ID

CODEOWNERS = ["@MomoRC-tech"]
DEPENDENCIES = ["i2c"]
AUTO_LOAD = ["sensor", "text_sensor"]

CONF_MPU_FFT_JSON_ID = "mpu_fft_json_id"
CONF_MAX_ANALYSIS_HZ = "max_analysis_hz"

mpu_fft_json_ns = cg.esphome_ns.namespace("mpu_fft_json")
MPUFftJsonComponent = mpu_fft_json_ns.class_(
    "MPUFftJsonComponent", cg.Component, i2c.I2CDevice
)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(MPUFftJsonComponent),
            cv.Optional(CONF_MAX_ANALYSIS_HZ, default=300.0): cv.float_,
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

    # Add the library dependency
    cg.add_library("arduinoFFT", "^2.0.4")
