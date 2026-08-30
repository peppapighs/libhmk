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

"""Run with python tools/metadata_test.py; no SCons or generated files needed."""

import contextlib
import gzip
import io
import json
import os
from pathlib import Path
import runpy
import sys
from types import ModuleType, SimpleNamespace
import unittest
from unittest.mock import Mock, mock_open, patch
from uuid import NAMESPACE_URL, UUID, uuid5


METADATA_SCRIPT = Path(__file__).resolve().parents[1] / "scripts" / "metadata.py"


class MetadataTest(unittest.TestCase):
    def generate(self, target="kbhe", vid="0xABCD", pid="0x12EF"):
        keyboard = SimpleNamespace(
            name="Test keyboard",
            usb=SimpleNamespace(vid=vid, pid=pid, port="hs"),
            keyboard=SimpleNamespace(
                num_profiles=2,
                num_layers=4,
                num_keys=1,
                num_advanced_keys=1,
                num_dynamic_keystroke_max_bindings=4,
                num_macro_nodes=32,
            ),
            layout=SimpleNamespace(
                model_dump=Mock(return_value={"keys": [{"x": 0, "y": 0}]})
            ),
        )
        slices = {}

        def to_slice_def(name, values):
            self.assertNotIn(name, slices)
            slices[name] = bytes(values) if isinstance(values, bytes) else list(values)
            return f"#define {name} {', '.join(str(value) for value in values)}"

        utils = ModuleType("utils")
        utils.get_kb_json = Mock(return_value=keyboard)
        utils.get_driver = Mock(return_value="test-driver")
        utils.get_adc_resolution = Mock(return_value=12)
        utils.resolve_default_keymaps = Mock(return_value=[[["KC_A"]]])
        utils.to_slice_def = to_slice_def
        schema = ModuleType("schema")
        schema_keyboard = ModuleType("schema.keyboard")
        schema_keyboard.KeyboardUSBPort = SimpleNamespace(HIGH_SPEED="hs")
        schema.keyboard = schema_keyboard
        modules = {"utils": utils, "schema": schema, "schema.keyboard": schema_keyboard}
        file_mock = mock_open()
        import_mock = Mock()

        with (
            patch.dict(sys.modules, modules),
            patch("builtins.open", file_mock),
            contextlib.redirect_stdout(io.StringIO()),
        ):
            runpy.run_path(
                str(METADATA_SCRIPT),
                init_globals={"Import": import_mock, "env": {"PIOENV": target}},
            )

        import_mock.assert_called_once_with("env")
        utils.get_kb_json.assert_called_once_with(target)
        utils.get_driver.assert_called_once_with(target)
        keyboard.layout.model_dump.assert_called_once_with(exclude_none=True)
        file_mock.assert_called_once_with(os.path.join("include", "metadata.h"), "w")
        file_mock().write.assert_called_once()
        header = file_mock().write.call_args.args[0]
        self.assertIn("#define MS_OS_20_GUID ", header)
        self.assertIn("#define KEYBOARD_METADATA ", header)
        return slices

    def guid_string(self, slices):
        encoded = slices["MS_OS_20_GUID"]
        self.assertEqual(len(encoded), 40)
        self.assertEqual(encoded[-2:], [r"U16_TO_U8S_LE('\0')"] * 2)
        prefix = "U16_TO_U8S_LE('"
        characters = []
        for token in encoded[:-2]:
            self.assertTrue(token.startswith(prefix) and token.endswith("')"))
            character = token[len(prefix):-2]
            self.assertEqual(len(character), 1)
            characters.append(character)
        return "".join(characters)

    def test_guid_is_stable_for_same_target(self):
        self.assertEqual(
            self.generate()["MS_OS_20_GUID"],
            self.generate()["MS_OS_20_GUID"],
        )

    def test_different_targets_with_same_usb_ids_have_different_guids(self):
        self.assertNotEqual(
            self.generate(target="kbhe")["MS_OS_20_GUID"],
            self.generate(target="another-keyboard")["MS_OS_20_GUID"],
        )

    def test_usb_id_case_does_not_change_guid(self):
        self.assertEqual(
            self.generate(vid="0xABCD", pid="0x12EF")["MS_OS_20_GUID"],
            self.generate(vid="0xabcd", pid="0x12ef")["MS_OS_20_GUID"],
        )

    def test_guid_format_identity_and_double_terminator(self):
        guid = self.guid_string(self.generate())
        self.assertRegex(
            guid,
            r"^\{[0-9A-F]{8}-[0-9A-F]{4}-5[0-9A-F]{3}-[89AB][0-9A-F]{3}-[0-9A-F]{12}\}$",
        )
        expected = uuid5(
            NAMESPACE_URL,
            "https://github.com/peppapighs/libhmk/keyboards/kbhe/usb/abcd:12ef/xinput",
        )
        self.assertEqual(UUID(guid), expected)

    def test_gzip_is_deterministic_and_advertises_both_gamepad_apis(self):
        with patch("gzip.compress", wraps=gzip.compress) as compress:
            first = self.generate()["KEYBOARD_METADATA"]
            second = self.generate()["KEYBOARD_METADATA"]
        self.assertEqual(compress.call_count, 2)
        for call in compress.call_args_list:
            self.assertEqual(call.kwargs, {"mtime": 0})
        self.assertEqual(first, second)
        self.assertEqual(first[4:8], bytes(4))
        metadata = json.loads(gzip.decompress(first))
        self.assertEqual(metadata["gamepadApis"], ["xinput", "hid"])
        self.assertTrue(metadata["usbHighSpeed"])
        self.assertEqual(metadata["defaultKeymaps"], [[["KC_A"]]])


if __name__ == "__main__":
    unittest.main()
