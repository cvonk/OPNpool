"""
@file __init__.py
@brief OPNpool - ESPHome Python codegen for the OPNpool component.
 
@details This file defines the ESPHome code generation logic for the OPNpool component,
which integrates an OPNpool interface with the ESPHome ecosystem. It declares the
configuration schema, entity types, and code generation routines for climate, switch,
sensor, binary sensor, and text sensor entities associated with the pool controller.

Responsibilities include: - Defining the configuration schema for all supported pool
entities and RS485 hardware settings. - Registering all subcomponents and their C++
counterparts for code generation. - Dynamically extracting the firmware version from Git
or ESPHome version for build metadata. - Adding required build flags and source files for
the C++ implementation. - Providing async routines to instantiate and link all entities to
the main OpnPool component.

WARNING: The script directly edits the `opnpool_ids.h` header file to keep the enums in
opnpool_ids.h consistent with CONF_* in this file.

This module enables seamless integration of pool automation hardware into ESPHome YAML
configurations, supporting flexible entity mapping and robust build-time configuration.

@author Coert Vonk (@cvonk on GitHub)
@copyright 2014, 2019, 2022, 2026, Coert Vonk
@license SPDX-License-Identifier: GPL-3.0-or-later
"""

import sys
if sys.version_info < (3, 6):
    raise RuntimeError("OPNpool ESPHome component requires Python 3.6 or newer.")

import os
import re
import subprocess
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import climate, switch, sensor, binary_sensor, text_sensor
from esphome.const import (
    CONF_ID,
    CONF_DEVICE_CLASS, DEVICE_CLASS_TEMPERATURE, DEVICE_CLASS_POWER, DEVICE_CLASS_VOLUME_FLOW_RATE, DEVICE_CLASS_EMPTY,
    CONF_UNIT_OF_MEASUREMENT, UNIT_CELSIUS, UNIT_WATT, UNIT_EMPTY, UNIT_REVOLUTIONS_PER_MINUTE, UNIT_PARTS_PER_MILLION, UNIT_PERCENT,
    CONF_STATE_CLASS, STATE_CLASS_MEASUREMENT
)

DEPENDENCIES = ["climate", "switch", "sensor", "binary_sensor", "text_sensor"]
AUTO_LOAD = ["climate", "switch", "sensor", "binary_sensor", "text_sensor"]

# namespace and class definitions
opnpool_ns = cg.esphome_ns.namespace("opnpool")
OpnPool             = opnpool_ns.class_("OpnPool", cg.Component)
OpnPoolClimate      = opnpool_ns.class_("OpnPoolClimate", climate.Climate, cg.Component)
OpnPoolSwitch       = opnpool_ns.class_("OpnPoolSwitch", switch.Switch, cg.Component)
OpnPoolSensor       = opnpool_ns.class_("OpnPoolSensor", sensor.Sensor, cg.Component)
OpnPoolBinarySensor = opnpool_ns.class_("OpnPoolBinarySensor", binary_sensor.BinarySensor, cg.Component)
OpnPoolTextSensor   = opnpool_ns.class_("OpnPoolTextSensor", text_sensor.TextSensor, cg.Component)

CONF_RS485         = "rs485"
CONF_RS485_RX_PIN  = "rx_pin"
CONF_RS485_TX_PIN  = "tx_pin"
CONF_RS485_RTS_PIN = "rts_pin"

# Matter over Thread configuration
CONF_MATTER               = "matter"
CONF_MATTER_ENABLED       = "enabled"
CONF_MATTER_DISCRIMINATOR = "discriminator"
CONF_MATTER_PASSCODE      = "passcode"

# Flash size configuration
CONF_FLASH_SIZE = "flash_size"
FLASH_SIZES = {
     "4MB":  4194304,
     "8MB":  8388608,
    "16MB": 16777216,
}

# MUST be in the same order as network_pool_thermo_t
CONF_CLIMATES = [  # used to overwrite climate_id_t enum in opnpool.h
    "pool_climate",
    "spa_climate"
]
# MUST be in the same order as network_pool_circuit_t
CONF_SWITCHES = [  # used to overwrite switch_id_t enum in opnpool.h
    "spa", 
    "aux1",
    "aux2",
    "aux3",
    "feature1",
    "pool",
    "feature2",
    "feature3",
    "feature4"
]
CONF_ANALOG_SENSORS = { # keys are used to overwrite sensor_id_t enum in opnpool.h
    "air_temperature":    {"unit": UNIT_CELSIUS, CONF_DEVICE_CLASS: DEVICE_CLASS_TEMPERATURE, CONF_STATE_CLASS: STATE_CLASS_MEASUREMENT},
    "water_temperature":  {"unit": UNIT_CELSIUS, CONF_DEVICE_CLASS: DEVICE_CLASS_TEMPERATURE, CONF_STATE_CLASS: STATE_CLASS_MEASUREMENT},
    "primary_pump_power": {"unit": UNIT_WATT, CONF_DEVICE_CLASS: DEVICE_CLASS_POWER, CONF_STATE_CLASS: STATE_CLASS_MEASUREMENT},
    "primary_pump_flow":  {"unit": "gal/min", CONF_DEVICE_CLASS: DEVICE_CLASS_VOLUME_FLOW_RATE, CONF_STATE_CLASS: STATE_CLASS_MEASUREMENT},
    "primary_pump_speed": {"unit": UNIT_REVOLUTIONS_PER_MINUTE, CONF_DEVICE_CLASS: DEVICE_CLASS_EMPTY, CONF_STATE_CLASS: STATE_CLASS_MEASUREMENT},
    "chlorinator_level":  {"unit": UNIT_PERCENT, CONF_DEVICE_CLASS: DEVICE_CLASS_EMPTY, CONF_STATE_CLASS: STATE_CLASS_MEASUREMENT},
    "chlorinator_salt":   {"unit": UNIT_PARTS_PER_MILLION, CONF_DEVICE_CLASS: DEVICE_CLASS_EMPTY, CONF_STATE_CLASS: STATE_CLASS_MEASUREMENT},
    "primary_pump_error": {"unit": UNIT_EMPTY, CONF_DEVICE_CLASS: DEVICE_CLASS_EMPTY, CONF_STATE_CLASS: STATE_CLASS_MEASUREMENT},
}
CONF_BINARY_SENSORS = [  # used to overwrite binary_sensor_id_t enum in opnpool.h
    "primary_pump_running",
    "mode_service",
    "mode_temperature_inc", 
    "mode_freeze_protection",
    "mode_timeout"
]
CONF_TEXT_SENSORS = [  # used to overwrite text_sensor_id_t enum in opnpool.h
    "pool_sched",
    "spa_sched",
    "primary_pump_mode",
    "primary_pump_state",
    "chlorinator_name",
    "chlorinator_status",
    "system_time",
    "controller_type",
    "interface_firmware"
]

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(OpnPool),
    # RS485 settings (required, but with defaults)
    cv.Optional(CONF_RS485, default={}): cv.Schema({
        cv.Optional(CONF_RS485_TX_PIN, default=21): cv.int_,
        cv.Optional(CONF_RS485_RX_PIN, default=22): cv.int_,
        cv.Optional(CONF_RS485_RTS_PIN, default=23): cv.int_,
    }),
    # Matter over Thread settings (optional, disabled by default)
    # Requires ESP32-C6 or ESP32-H2 with Thread radio support
    cv.Optional(CONF_MATTER, default={}): cv.Schema({
        cv.Optional(CONF_MATTER_ENABLED, default=False): cv.boolean,
        cv.Optional(CONF_MATTER_DISCRIMINATOR, default=3840): cv.int_range(min=0, max=4095),
        cv.Optional(CONF_MATTER_PASSCODE, default=20202021): cv.int_range(min=1, max=99999998),
    }),
    # Flash size (ESP32-C6-DevKitC-1-N8 has 8MB, some variants have 4MB)
    cv.Optional(CONF_FLASH_SIZE, default="8MB"): cv.one_of(*FLASH_SIZES.keys(), upper=True),
    **{
        cv.Optional(key, default={"name": key.replace("_", " ").title()}): climate.climate_schema(OpnPoolClimate).extend({
            cv.GenerateID(): cv.declare_id(OpnPoolClimate)
        }) for key in CONF_CLIMATES
    },
    **{
        cv.Optional(key, default={"name": key.replace("_", " ").title()}): switch.switch_schema(OpnPoolSwitch).extend({
            cv.GenerateID(): cv.declare_id(OpnPoolSwitch)
        }) for key in CONF_SWITCHES
    },
    **{
        cv.Optional(key, default={"name": key.replace("_", " ").title()}): sensor.sensor_schema(OpnPoolSensor).extend({
            cv.GenerateID(): cv.declare_id(OpnPoolSensor),
            cv.Optional(CONF_UNIT_OF_MEASUREMENT, default=CONF_ANALOG_SENSORS[key]["unit"]): cv.string,
            cv.Optional(CONF_DEVICE_CLASS, default=CONF_ANALOG_SENSORS[key][CONF_DEVICE_CLASS]): cv.string,
        }) for key in CONF_ANALOG_SENSORS
    },
    **{
        cv.Optional(key, default={"name": key.replace("_", " ").title()}): binary_sensor.binary_sensor_schema(OpnPoolBinarySensor).extend({
            cv.GenerateID(): cv.declare_id(OpnPoolBinarySensor)
        }) for key in CONF_BINARY_SENSORS
    },
    **{
        cv.Optional(key, default={"name": key.replace("_", " ").title()}): text_sensor.text_sensor_schema(OpnPoolTextSensor).extend({
            cv.GenerateID(): cv.declare_id(OpnPoolTextSensor)
        }) for key in CONF_TEXT_SENSORS
    },
}).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    """Generate C++ code for the OPNpool ESPHome component.

    This async function is called by ESPHome's code generation framework to produce
    the C++ code that implements the OPNpool component. It instantiates the main
    OpnPool component, configures RS485 pins, registers all entity types (climate,
    switch, sensor, binary sensor, text sensor), and adds necessary build flags.

    Args:
        config: The validated configuration dictionary from the ESPHome YAML.
    """
    # instantiate the main OpnPool component
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    # add all source files to build
    cg.add_library("ESP32", None, "freertos")

    # component directory (used for include path and source file discovery)
    component_dir = os.path.dirname(os.path.abspath(__file__))
    component_dir_cmake = component_dir.replace("\\", "/")

    # add component directory to include path so subdirectory files can
    # cross-reference each other with includes like "pool_task/network.h"
    cg.add_build_flag(f"-I{component_dir_cmake}")

    # C++ implementation files — use absolute paths because ESPHome only copies
    # root-level files to the build src/ directory; subdirectory files stay at
    # the original component location and must be referenced by absolute path.
    cg.add_platformio_option("build_src_filter", [
        "+<*>",
        f"+<{component_dir_cmake}/core/*.cpp>",
        f"+<{component_dir_cmake}/entities/*.cpp>",
        f"+<{component_dir_cmake}/ipc/*.cpp>",
        f"+<{component_dir_cmake}/pool_task/*.cpp>",
        f"+<{component_dir_cmake}/utils/*.cpp>",
        f"+<{component_dir_cmake}/matter/*.cpp>",
    ])
    partitions_path = os.path.join(component_dir, "core", "partitions.csv").replace("\\", "/")
    cg.add_platformio_option("board_build.partitions", partitions_path)

    # Flash size configuration (override board defaults)
    flash_size = config[CONF_FLASH_SIZE]
    flash_size_bytes = FLASH_SIZES[flash_size]
    cg.add_platformio_option("board_build.flash_size",    flash_size)
    cg.add_platformio_option("board_upload.flash_size",   flash_size)
    cg.add_platformio_option("board_upload.maximum_size", flash_size_bytes)

    # add build flags
    cg.add_build_flag("-fmax-errors=5")
    cg.add_build_flag("-Wl,-Map=output.map")

    # interface firmware version
    version = "unknown"
    try:
        git_hash = subprocess.check_output(
            ['git', 'rev-parse', '--short', 'HEAD'],
            cwd=component_dir,
            stderr=subprocess.DEVNULL
        ).decode('ascii').strip()

        try:
            subprocess.check_output(
                ['git', 'diff', '--quiet'],
                cwd=component_dir,
                stderr=subprocess.DEVNULL
            )
            dirty = ""
        except subprocess.CalledProcessError:
            dirty = "-dirty"
        
        version = f"git-{git_hash}{dirty}"
    except Exception:
        try:
            from esphome.const import __version__ as ESPHOME_VERSION
            version = f"esphome-{ESPHOME_VERSION}"
        except Exception:
            import logging
            logging.warning("OPNpool: Could not determine firmware version from Git or ESPHome. Using 'unknown'.")
            version = "unknown"

    cg.add_build_flag(f"-DGIT_HASH=\\\"{version}\\\"")
    
    # RS485 configuration
    rs485_config = config[CONF_RS485]
    cg.add(var.set_rs485_pins(rs485_config[CONF_RS485_RX_PIN], rs485_config[CONF_RS485_TX_PIN], rs485_config[CONF_RS485_RTS_PIN]))

    # matter over Thread configuration
    matter_config = config[CONF_MATTER]
    if matter_config[CONF_MATTER_ENABLED]:
        import logging

        cg.add_define("USE_MATTER")
        cg.add_build_flag("-DCONFIG_ENABLE_MATTER=1")
        cg.add(var.set_matter_config(
            matter_config[CONF_MATTER_DISCRIMINATOR],
            matter_config[CONF_MATTER_PASSCODE]
        ))

        # log Matter configuration
        # NOTE: esp-matter integration requires manual setup:
        #   1. Copy idf_component.yml to the ESPHome build directory's main component
        #   2. Run 'idf.py reconfigure' to download esp-matter
        #   3. Rebuild the project
        # See: https://components.espressif.com/components/espressif/esp_matter
        logging.info("OPNpool: Matter enabled with discriminator=%d", matter_config[CONF_MATTER_DISCRIMINATOR])
        logging.warning("OPNpool: Matter requires manual esp-matter setup. See idf_component.yml in project root")

    # register climate entities (constructor injection)
    for id, climate_key in enumerate(CONF_CLIMATES):
        entity_cfg = config[climate_key]
        if CONF_ID not in entity_cfg:
            entity_cfg[CONF_ID] = cg.new_id()
        climate_entity = cg.new_Pvariable(entity_cfg[CONF_ID], var, id)
        await climate.register_climate(climate_entity, entity_cfg)
        cg.add(getattr(var, f"set_{climate_key}")(climate_entity))

    # register switches (constructor injection)
    for id, switch_key in enumerate(CONF_SWITCHES):
        entity_cfg = config[switch_key]
        if CONF_ID not in entity_cfg:
            entity_cfg[CONF_ID] = cg.new_id()
        switch_entity = cg.new_Pvariable(entity_cfg[CONF_ID], var, id)
        await switch.register_switch(switch_entity, entity_cfg)
        cg.add(getattr(var, f"set_{switch_key}_switch")(switch_entity))

    # register analog sensors (constructor injection)
    for id, sensor_key in enumerate(CONF_ANALOG_SENSORS):
        entity_cfg = config[sensor_key]
        if CONF_ID not in entity_cfg:
            entity_cfg[CONF_ID] = cg.new_id()
        sensor_entity = cg.new_Pvariable(entity_cfg[CONF_ID])
        await sensor.register_sensor(sensor_entity, entity_cfg)
        cg.add(getattr(var, f"set_{sensor_key}_sensor")(sensor_entity))

    # register binary sensors (constructor injection)
    for id, binary_sensor_key in enumerate(CONF_BINARY_SENSORS):
        entity_cfg = config[binary_sensor_key]
        if CONF_ID not in entity_cfg:
            entity_cfg[CONF_ID] = cg.new_id()
        bs_entity = cg.new_Pvariable(entity_cfg[CONF_ID])
        await binary_sensor.register_binary_sensor(bs_entity, entity_cfg)
        cg.add(getattr(var, f"set_{binary_sensor_key}_binary_sensor")(bs_entity))

    # register text sensors (constructor injection)
    for id, text_sensor_key in enumerate(CONF_TEXT_SENSORS):
        entity_cfg = config[text_sensor_key]
        if CONF_ID not in entity_cfg:
            entity_cfg[CONF_ID] = cg.new_id()
        ts_entity = cg.new_Pvariable(entity_cfg[CONF_ID])
        await text_sensor.register_text_sensor(ts_entity, entity_cfg)
        cg.add(getattr(var, f"set_{text_sensor_key}_text_sensor")(ts_entity))

# replace the enums in opnpool_ids.h to keep them consistent with CONF_* in this file

ENTITY_ENUMS = {
    "climate_id_t":       CONF_CLIMATES,
    "switch_id_t":        CONF_SWITCHES,
    "sensor_id_t":        CONF_ANALOG_SENSORS,
    "binary_sensor_id_t": CONF_BINARY_SENSORS,
    "text_sensor_id_t":   CONF_TEXT_SENSORS,
}

def generate_enum(enum_name, items):
    """Generate a C++ enum class definition from a list of items.

    Creates a C++ enum class with uint8_t underlying type, where each item
    is assigned a sequential value starting from 0.

    Args:
        enum_name: The name for the C++ enum class (e.g., "climate_id_t").
        items: List of enum member names in lowercase with underscores.

    Returns:
        A string containing the complete C++ enum class definition.
    """
    lines = [f"enum class {enum_name} : uint8_t {{"]
    for id, name in enumerate(items):
        lines.append(f"    {name.upper()} = {id},")
    lines.append("};")
    return "\n".join(lines)

def update_header(header_path, entity_enums):
    """Update C++ header file with regenerated enum definitions.

    Reads the specified header file, replaces all entity enum definitions with
    newly generated versions based on the Python CONF_* lists, and atomically
    writes the updated content back to the file.

    This ensures the C++ enums in opnpool_ids.h stay synchronized with the
    entity configurations defined in this Python module.

    Args:
        header_path: Absolute path to the C++ header file to update.
        entity_enums: Dict mapping enum names to lists of enum member names.
    """
    import tempfile
    import shutil

    with open(header_path, "r") as f:
        content = f.read()
    for enum_name, items in entity_enums.items():
        # accept both old and new enum names for replacement, e.g., switch_id_t or SwitchId
        pattern = rf"enum class (?:{enum_name}|{enum_name.replace('_id_t','Id').replace('_','').capitalize()}) : uint8_t \{{.*?\}};"
        new_enum = generate_enum(enum_name, items)
        content = re.sub(pattern, new_enum, content, flags=re.DOTALL)

    # write to a temporary file first
    dir_name = os.path.dirname(header_path)
    with tempfile.NamedTemporaryFile("w", dir=dir_name, delete=False) as tmp_file:
        tmp_file.write(content)
        temp_path = tmp_file.name

    # atomically replace the original file
    shutil.move(temp_path, header_path)

if __name__ == "__main__":
    script_dir = os.path.dirname(os.path.abspath(__file__))
    header_path = os.path.join(script_dir, "core", "opnpool_ids.h")
    update_header(header_path, ENTITY_ENUMS)
