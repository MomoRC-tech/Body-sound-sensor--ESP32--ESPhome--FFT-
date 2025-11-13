"""Text sensor platform for MPU FFT JSON component."""
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text_sensor
from esphome.const import CONF_ID
from . import MPUFftJsonComponent, CONF_MPU_FFT_JSON_ID, mpu_fft_json_ns

CONF_SPECTRUM = "spectrum"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_MPU_FFT_JSON_ID): cv.use_id(MPUFftJsonComponent),
        cv.Optional(CONF_SPECTRUM): text_sensor.text_sensor_schema(),
    }
)


async def to_code(config):
    """Code generation for MPU FFT JSON text sensor."""
    parent = await cg.get_variable(config[CONF_MPU_FFT_JSON_ID])

    if spectrum_config := config.get(CONF_SPECTRUM):
        sens = await text_sensor.new_text_sensor(spectrum_config)
        cg.add(parent.set_spectrum_text_sensor(sens))
