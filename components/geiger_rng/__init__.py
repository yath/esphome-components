from esphome import pins
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID, CONF_INTERNAL_FILTER, CONF_PIN, CONF_PORT

DEPENDENCIES = ["network"]
AUTO_LOAD = ["async_tcp"]

GeigerRNGComponent = cg.global_ns.class_("GeigerRNGComponent", cg.Component)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(GeigerRNGComponent),
            cv.Required(CONF_PIN): pins.internal_gpio_input_pin_schema,
            cv.Optional(CONF_PORT, default=19): cv.port,
            cv.Optional(CONF_INTERNAL_FILTER, default="13us"): cv.positive_time_period_microseconds,
        }
    ).extend(cv.COMPONENT_SCHEMA)
)

async def to_code(config):
    pin = await cg.gpio_pin_expression(config[CONF_PIN])
    var = cg.new_Pvariable(config[CONF_ID], pin, config[CONF_PORT], config[CONF_INTERNAL_FILTER])
    return await cg.register_component(var, config)
