"""MPU FFT JSON Component for ESPHome."""
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import i2c
from esphome.const import CONF_ID

CODEOWNERS = ["@MomoRC-tech"]
DEPENDENCIES = ["i2c"]
AUTO_LOAD = ["sensor", "text_sensor"]

CONF_MPU_FFT_JSON_ID = "mpu_fft_json_id"

mpu_fft_json_ns = cg.esphome_ns.namespace("mpu_fft_json")
MPUFftJsonComponent = mpu_fft_json_ns.class_(
    "MPUFftJsonComponent", cg.Component, i2c.I2CDevice
)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(MPUFftJsonComponent),
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

    # Add the library dependency
    cg.add_library("arduinoFFT", "^2.0.4")
