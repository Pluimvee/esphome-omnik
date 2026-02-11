"""ESPHome component: omnik_bridge - Omnik Inverter WiFi Bridge."""
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor, binary_sensor, text_sensor
from esphome.const import (
    CONF_ID,
    CONF_PORT,
    CONF_CHANNEL,
    CONF_PASSWORD,
    CONF_TEMPERATURE,
    CONF_POWER,
    UNIT_CELSIUS,
    UNIT_WATT,
    UNIT_HERTZ,
    UNIT_VOLT,
    UNIT_KILOWATT_HOURS,
    UNIT_HOUR,
    UNIT_AMPERE,
    DEVICE_CLASS_TEMPERATURE,
    DEVICE_CLASS_POWER,
    DEVICE_CLASS_FREQUENCY,
    DEVICE_CLASS_VOLTAGE,
    DEVICE_CLASS_ENERGY,
    DEVICE_CLASS_DURATION,
    DEVICE_CLASS_CURRENT,
    DEVICE_CLASS_CONNECTIVITY,
    STATE_CLASS_MEASUREMENT,
    STATE_CLASS_TOTAL_INCREASING,
)

CODEOWNERS = ["@Pluimvee"]
AUTO_LOAD = ["sensor", "binary_sensor", "text_sensor"]

CONF_AP_SSID = "ap_ssid"
CONF_AP_PASSWORD = "ap_password"
CONF_AP_IP = "ap_ip"
CONF_AP_SUBNET = "ap_subnet"
CONF_AC_FREQUENCY = "ac_frequency"
CONF_AC_VOLTAGE = "ac_voltage"
CONF_ENERGY_TODAY = "energy_today"
CONF_ENERGY_TOTAL = "energy_total"
CONF_OPERATING_HOURS = "operating_hours"
CONF_VOLTAGE_PV1 = "voltage_pv1"
CONF_CURRENT_PV1 = "current_pv1"
CONF_CURRENT_AC1 = "current_ac1"
CONF_CONNECTED = "connected"
CONF_LOGGER_ID = "logger_id"
CONF_INVERTER_ID = "inverter_id"

omnik_bridge_ns = cg.esphome_ns.namespace("omnik_bridge")
OmnikBridge = omnik_bridge_ns.class_("OmnikBridge", cg.Component)

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(OmnikBridge),
    cv.Required(CONF_AP_SSID): cv.string,
    cv.Required(CONF_AP_PASSWORD): cv.string,
    cv.Required(CONF_AP_IP): cv.string,
    cv.Optional(CONF_AP_SUBNET, default="255.255.255.0"): cv.string,
    cv.Optional(CONF_PORT, default=10004): cv.port,
    cv.Optional(CONF_CHANNEL, default=0): cv.int_range(min=0, max=14),

    cv.Optional(CONF_TEMPERATURE): sensor.sensor_schema(
        unit_of_measurement=UNIT_CELSIUS,
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_TEMPERATURE,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    cv.Optional(CONF_POWER): sensor.sensor_schema(
        unit_of_measurement=UNIT_WATT,
        accuracy_decimals=0,
        device_class=DEVICE_CLASS_POWER,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    cv.Optional(CONF_AC_FREQUENCY): sensor.sensor_schema(
        unit_of_measurement=UNIT_HERTZ,
        accuracy_decimals=2,
        device_class=DEVICE_CLASS_FREQUENCY,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    cv.Optional(CONF_AC_VOLTAGE): sensor.sensor_schema(
        unit_of_measurement=UNIT_VOLT,
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_VOLTAGE,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    cv.Optional(CONF_ENERGY_TODAY): sensor.sensor_schema(
        unit_of_measurement=UNIT_KILOWATT_HOURS,
        accuracy_decimals=2,
        device_class=DEVICE_CLASS_ENERGY,
        state_class=STATE_CLASS_TOTAL_INCREASING,
    ),
    cv.Optional(CONF_ENERGY_TOTAL): sensor.sensor_schema(
        unit_of_measurement=UNIT_KILOWATT_HOURS,
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_ENERGY,
        state_class=STATE_CLASS_TOTAL_INCREASING,
    ),
    cv.Optional(CONF_OPERATING_HOURS): sensor.sensor_schema(
        unit_of_measurement=UNIT_HOUR,
        accuracy_decimals=0,
        device_class=DEVICE_CLASS_DURATION,
        state_class=STATE_CLASS_TOTAL_INCREASING,
    ),
    cv.Optional(CONF_VOLTAGE_PV1): sensor.sensor_schema(
        unit_of_measurement=UNIT_VOLT,
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_VOLTAGE,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    cv.Optional(CONF_CURRENT_PV1): sensor.sensor_schema(
        unit_of_measurement=UNIT_AMPERE,
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_CURRENT,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    cv.Optional(CONF_CURRENT_AC1): sensor.sensor_schema(
        unit_of_measurement=UNIT_AMPERE,
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_CURRENT,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    cv.Optional(CONF_CONNECTED): binary_sensor.binary_sensor_schema(
        device_class=DEVICE_CLASS_CONNECTIVITY,
    ),
    cv.Optional(CONF_LOGGER_ID): text_sensor.text_sensor_schema(),
    cv.Optional(CONF_INVERTER_ID): text_sensor.text_sensor_schema(),
})


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    cg.add(var.set_ap_ssid(config[CONF_AP_SSID]))
    cg.add(var.set_ap_password(config[CONF_AP_PASSWORD]))
    cg.add(var.set_ap_ip(config[CONF_AP_IP]))
    cg.add(var.set_ap_subnet(config[CONF_AP_SUBNET]))
    cg.add(var.set_port(config[CONF_PORT]))
    cg.add(var.set_channel(config[CONF_CHANNEL]))

    for key, setter in [
        (CONF_TEMPERATURE, var.set_temperature_sensor),
        (CONF_POWER, var.set_power_sensor),
        (CONF_AC_FREQUENCY, var.set_ac_frequency_sensor),
        (CONF_AC_VOLTAGE, var.set_ac_voltage_sensor),
        (CONF_ENERGY_TODAY, var.set_energy_today_sensor),
        (CONF_ENERGY_TOTAL, var.set_energy_total_sensor),
        (CONF_OPERATING_HOURS, var.set_operating_hours_sensor),
        (CONF_VOLTAGE_PV1, var.set_voltage_pv1_sensor),
        (CONF_CURRENT_PV1, var.set_current_pv1_sensor),
        (CONF_CURRENT_AC1, var.set_current_ac1_sensor),
    ]:
        if key in config:
            sens = await sensor.new_sensor(config[key])
            cg.add(setter(sens))

    if CONF_CONNECTED in config:
        sens = await binary_sensor.new_binary_sensor(config[CONF_CONNECTED])
        cg.add(var.set_connected_sensor(sens))

    if CONF_LOGGER_ID in config:
        sens = await text_sensor.new_text_sensor(config[CONF_LOGGER_ID])
        cg.add(var.set_logger_id_sensor(sens))

    if CONF_INVERTER_ID in config:
        sens = await text_sensor.new_text_sensor(config[CONF_INVERTER_ID])
        cg.add(var.set_inverter_id_sensor(sens))
