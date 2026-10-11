# STM32F723 CRC contract test

This host-only test compiles the real driver against minimal HAL stubs. It checks
the F7 input format, default CRC settings, seed handling, and zero-padded tails
for lengths 0 through 12. It is not a hardware/peripheral validation.

Run from the repository root with a native C compiler (Linux/WSL):

```sh
cc -std=c11 -Wall -Wextra -Werror -Itests/stm32f723_crc/stubs \
  tests/stm32f723_crc/test.c src/hardware/stm32f723xx/crc32.c \
  -o /tmp/stm32f723_crc_test
/tmp/stm32f723_crc_test
```
