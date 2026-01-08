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


class CompilerFlags:
    """
    A class to manage compiler flags for building the firmware.
    """

    def __init__(self):
        self.flags = []

    def get_flags(self):
        """
        Get the list of compiler flags.
        """
        return self.flags

    def define(self, name: str, value: str = None):
        """
        Add a preprocessor definition to the compiler flags.
        """
        if value is None:
            self.flags.append(f"-D{name}")
        else:
            self.flags.append(f"-D{name}='{value}'")

    def include(self, path: str):
        """
        Add an include path to the compiler flags.
        """
        self.flags.append(f"-I{path}")


def get_kb_json(keyboard: str):
    """
    Load the keyboard JSON configuration file.
    """
    with open(os.path.join("keyboards", keyboard, "keyboard.json"), "r") as f:
        return json.load(f)


def get_driver_json(keyboard: str):
    """
    Load the driver JSON configuration file.
    """
    kb_json = get_kb_json(keyboard)
    with open(
        os.path.join("hardware", kb_json["hardware"]["driver"], "info.json"), "r"
    ) as f:
        return json.load(f)


def to_c_array(arr: list):
    """
    Convert a Python list to a C array initializer.
    """
    return f"{{{', '.join(to_c_array(x) if isinstance(x, list) else str(x) for x in arr)}}}"


def to_c_struct(value: dict):
    """
    Convert a Python dictionary to a C struct initializer.
    """
    return f"{{{', '.join(f'.{k} = {v}' for k, v in value.items())}}}"


def to_gpio_array(pins: list[str], driver: str):
    """
    Convert a list of GPIO pin names to a tuple of (port names, pin numbers)
    """
    if driver == "stm32f446xx":
        ports = [f"GPIO{pin[0]}" for pin in pins]
        pin_nums = [f"GPIO_PIN_{pin[1:]}" for pin in pins]
    elif driver == "at32f405xx":
        ports = [f"GPIO{pin[0]}" for pin in pins]
        pin_nums = [f"GPIO_PINS_{pin[1:]}" for pin in pins]
    else:
        raise ValueError(f"Unsupported driver: {driver}")
    return (ports, pin_nums)


def to_adc_input_array(channels_or_pins: list[str | int], driver_json: dict):
    """
    Convert a list of ADC input channels or GPIO pin names to a C array of ADC input channels
    """
    adc_input_pins = driver_json["metadata"]["adc_input_pins"]
    channels = []
    for x in channels_or_pins:
        if isinstance(x, str):
            channels.append(adc_input_pins.index(x))
        else:
            assert x < len(adc_input_pins)
            channels.append(x)
    return to_c_array(channels)


def get_adc_resolution(kb_json: dict, driver_json: dict):
    """
    Get the ADC resolution, or default to the maximum resolution supported by the MCU
    """
    return (
        kb_json["analog"]["adc_resolution"]
        if "adc_resolution" in kb_json["analog"]
        else driver_json["metadata"]["max_adc_resolution"]
    )


FLASH_SECTOR_SIZES = {
    # Sector layout for STM32F446RE: 4x16K, 1x64K, 3x128K
    "stm32f446xx": [
        16 * 1024,
        16 * 1024,
        16 * 1024,
        16 * 1024,
        64 * 1024,
        128 * 1024,
        128 * 1024,
        128 * 1024,
    ],
    # AT32F405RC: 128 uniform 2K sectors (256K total)
    "at32f405xx": [2048] * 128,
}


def get_flash_sector_sizes(driver: str) -> list[int]:
    """
    Get the flash sector sizes for the given driver.
    """
    if driver not in FLASH_SECTOR_SIZES:
        raise ValueError(f"Unsupported driver: {driver}")
    return FLASH_SECTOR_SIZES[driver]


def round_up_to_flash_sectors(required_size: int, sector_sizes: list[int]) -> int:
    """
    Round up the required size to the minimum number of flash sectors (from the end).
    """
    total = 0
    for size in reversed(sector_sizes):
        total += size
        if total >= required_size:
            return total
    return total
