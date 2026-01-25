# This program is free software: you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free Software
# Foundation, either version 3 of the License, or (at your option) any later
# version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
# details.
#
# You should have received a copy of the GNU General Public License along with
# this program. If not, see <https://www.gnu.org/licenses/>.

import json
import os
from drivers import *


class CompilerFlags:
    def __init__(self):
        self.flags = []

    # Get the list of compiler flags
    def get_flags(self):
        return self.flags

    # Add a preprocessor definition to the compiler flags
    def define(self, name: str, value: int | str | None = None):
        match value:
            case None:
                self.flags.append(f"-D{name}")
            case int() | str():
                self.flags.append(f"-D{name}='{value}'")

    # Add a linker definition to the compiler flags
    def linker_defsym(self, name: str, value: int | str | None = None):
        match value:
            case None:
                self.flags.append(f"-Wl,--defsym,{name}")
            case int() | str():
                self.flags.append(f"-Wl,--defsym,{name}={value}")

    # Add an include path to the compiler flags
    def include(self, path: str):
        self.flags.append(f"-I{path}")


# Load the keyboard JSON configuration file
def get_kb_json(keyboard: str):
    with open(os.path.join("keyboards", keyboard, "keyboard.json"), "r") as f:
        return json.load(f)


# Load the driver based on the keyboard configuration
def get_driver(keyboard: str):
    kb_json = get_kb_json(keyboard)
    driver = kb_json["hardware"]["driver"]
    match driver:
        case "stm32f446xx":
            return STM32F446XX
        case "at32f405xx":
            return AT32F405XX
        case _:
            raise ValueError(f"Unsupported driver: {driver}")


# Convert a Python list to a C array initializer
def to_c_array(arr: list | bytes):
    return f"{{{', '.join(to_c_array(x) if isinstance(x, list) else str(x) for x in arr)}}}"


# Convert a Python dictionary to a C struct initializer
def to_c_struct(value: dict):
    return f"{{{', '.join(f'.{k} = {v}' for k, v in value.items())}}}"


# Convert a Pyhon list to a C array slice definition
def to_slice_def(name: str, arr: list | bytes):
    return f"#define {name.upper()} {', '.join(str(x) for x in arr)}"


# Get the ADC resolution, or default to the maximum resolution supported by the MCU
def get_adc_resolution(kb_json: dict, driver: Driver):
    return kb_json["analog"].get("adc_resolution", driver.metadata.adc.max_resolution)


# Resolve per-profile default keymaps
def resolve_default_keymaps(kb_json: dict) -> list[list[list[str]]]:
    if "keymaps" in kb_json:
        return kb_json["keymaps"]
    else:
        # Fallback to default keymap
        if "keymap" not in kb_json:
            raise ValueError(
                "Default keymap must be specified when no per-profile default keymaps are specified"
            )
        return [kb_json["keymap"]] * kb_json["keyboard"]["num_profiles"]
