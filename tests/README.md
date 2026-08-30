# Host regression tests

Run from the repository root. No keyboard is required.

Metadata generation (Python standard library only):

```sh
python3 tools/metadata_test.py
```

Gamepad options and configuration migration (Linux/WSL, native C compiler):

```sh
cc -std=gnu11 -Wall -Wextra -Werror -Wsign-conversion -Wno-unused-parameter \
  -I tests -I include tests/gamepad_options_test.c src/migration.c \
  -o /tmp/libhmk-gamepad-options-test
/tmp/libhmk-gamepad-options-test
```
