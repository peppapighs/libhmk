# libhmk

This repository contains libraries for building a Hall-effect keyboard firmware.

## Table of Contents

- [Features](#features)
- [Limitations](#limitations)
- [Getting Started](#getting-started)
- [Porting](#porting)
- [Acknowledgements](#acknowledgements)

## Features

- [x] **Analog Input**: Customizable actuation point for each key and many other features.
- [x] **Rapid Trigger**: Register a key press or release based on the change in key position and the direction of that change
- [x] **Continuous Rapid Trigger**: Deactivate Rapid Trigger only when the key is fully released.
- [x] **Null Bind (SOCD + Rappy Snappy)**: Monitor 2 keys and select which one is active based on the chosen behavior.
- [x] **Dynamic Keystroke**: Assign up to 4 keycodes to a single key. Each keycode can be assigned up to 4 actions for 4 different parts of the keystroke.
- [x] **Tap-Hold**: Send a different keycode depending on whether the key is tapped or held.
- [x] **Toggle**: Toggle between key press and key release. Hold the key for normal behavior.
- [x] **N-Key Rollover**: Support for N-Key Rollover and automatically fall back to 6-Key Rollover in BIOS.
- [x] **Automatic Calibration**: Automatically calibrate the analog input without requiring user intervention.
- [x] **EEPROM Emulation**: No external EEPROM required. Emulate EEPROM using the internal flash memory.
- [x] **Web Configurator**: Configure the firmware using [hmkconf](https://github.com/peppapighs/hmkconf) without needing to recompile the firmware.
- [ ] **Bootloader**: Optional UF2 bootloader for firmware updates over USB.
- [ ] **Gamepad**: Support for gamepad functionality.
- [ ] **Tick Rate**: Customizable tick rate for Tap-Hold and Dynamic Keystroke.

## Limitations

- **Polling Rate**: The firmware is designed to run at 1 kHz polling rate.
- **RGB Lighting**: The firmware does not support RGB lighting.

## Getting Started

### Prerequisites

- [PlatformIO](https://platformio.org/)
- [Python 3](https://www.python.org/) (Optional, for generating the distance lookup table)

### Building the Firmware

1. Clone the repository:

```bash
git clone https://github.com/peppapighs/libhmk.git
```

2. Open the project in PlatformIO, for example, through Visual Studio Code.

3. Create a directory for your keyboard under `src/keyboards/` and create a `config.h` file with the required configurations. An example [`keyboards/gauss64/config.h`](keyboards/gauss64/config.h) is provided. See the header files in [`include/`](include/) and [`hardware/`](hardware/) for the available options.

4. Configure the `platformio.ini` file for your keyboard. An example configuration is available at `keyboards/<KEYBOAD_NAME>/pio.ini`. The configuration file includes the following sections:

   - `env`: Shared configuration for all environments. The following options are configurable:
     - `debug_tool` (Optional): See [Debugging](https://docs.platformio.org/en/latest/projectconf/sections/env/options/debug/index.html).
     - `upload_protocol` (Optional): See [Upload](https://docs.platformio.org/en/latest/projectconf/sections/env/options/upload/index.html).
   - `env:<YOUR_ENVIRONMENT_NAME>`: Configuration specific to your keyboard. An example `env:gauss64` is provided. The following options are configurable:
     - `board`: The microcontroller used by your keyboard. See [Platform](https://docs.platformio.org/en/latest/projectconf/sections/env/options/platform/index.html).
     - `board_build.f_cpu` (Optional): The clock frequency of the microcontroller if it is not the default value. See [Platform](https://docs.platformio.org/en/latest/projectconf/sections/env/options/platform/index.html).
     - `build_flags`: Flags passed to the compiler. Include the path to the hardware-specific header files and the configuration file for your keyboard that you created in the previous step. See [Build](https://docs.platformio.org/en/latest/projectconf/sections/env/options/build/index.html).
     - `build_src_filter`: Filter for the source files. Include the path to the hardware-specific source files. See [Build](https://docs.platformio.org/en/latest/projectconf/sections/env/options/build/index.html).
     - `framework`: Framework used by the implementation of the hardware-specific code. See [Platform](https://docs.platformio.org/en/latest/projectconf/sections/env/options/platform/index.html).
     - `platform`: Platform used by the implementation of the hardware-specific code. See [Platform](https://docs.platformio.org/en/latest/projectconf/sections/env/options/platform/index.html).

5. (Optional) Since the relationship between the ADC value and the key travel distance is non-linear, a lookup table is used to assist the conversion between the two. The table can be found in [`include/distance.h`](include/distance.h). The table `distance_table` is generated by running the Python script [`distance_lut.py`](distance_lut.py). To adjust the table for your keyboard, you should sample the ADC values at various key travel distances, and add the samples to [this Desmos graph](https://www.desmos.com/calculator/nzl6twp6ui). The samples can be obtained from the Debug tab of [hmkconf](https://github.com/peppapighs/hmkconf). You are minimally required to add the samples for a fully released key, and a fully pressed key. Fit the curve to obtain the coefficient `a`, which is required to run the script. The script will generate the table for you to replace the existing one.

6. Build the firmware using PlatformIO.

7. Flash the firmware to your keyboard through PlatformIO, the bootloader of your keyboard, or any other method.

8. Configure the keyboard using [hmkconf](https://github.com/peppapighs/hmkconf).

## Porting

See [`hardware/stm32f446xx`](hardware/stm32f446xx/) and [`src/hardware/stm32f446xx`](src/hardware/stm32f446xx/) for an example implementation of the hardware-specific code for the STM32F446xx microcontroller.

## Acknowledgements

- [hathach/tinyusb](https://github.com/hathach/tinyusb) for the USB stack.
- [qmk/qmk_firmware](https://github.com/qmk/qmk_firmware) for inspiration, including EEPROM emulation and matrix scanning.
- [@riskable](https://github.com/riskable) for pioneering custom Hall-effect keyboard firmware development.
- [@heiso](https://github.com/heiso/) for his [macrolev](https://github.com/heiso/macrolev) and his helpfulness throughout the development process.
- [Wooting](https://wooting.io/) for pioneering Hall-effect gaming keyboards and introducing many advanced features based on analog input.
- [GEONWORKS](https://geon.works/) for the Venom 60HE PCB and inspiring the web configurator.
