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
import utils as utils
from drivers import Driver

Import("env")

PIP_PACKAGES = ["pip", "jsonschema"]

for package in PIP_PACKAGES:
    env.Execute(f"$PYTHONEXE -m pip install {package} --upgrade")

keyboard = env["PIOENV"]

# Load driver (only for validation)
_: Driver = utils.get_driver(keyboard)

# Load JSON files and validate
import jsonschema

kb_json = utils.get_kb_json(keyboard)
with open(os.path.join("scripts", "schema", "keyboard.schema.json")) as f:
    kb_schema = json.load(f)
jsonschema.validate(kb_json, schema=kb_schema)

# Validate default keymaps
default_keymaps = utils.resolve_default_keymaps(kb_json)
if len(default_keymaps) != kb_json["keyboard"]["num_profiles"]:
    raise ValueError(
        f"Expected default keymaps to have {kb_json['keyboard']['num_profiles']} profiles"
    )
for keymap in default_keymaps:
    if len(keymap) != kb_json["keyboard"]["num_layers"]:
        raise ValueError(
            f"Expected default keymaps to have {kb_json['keyboard']['num_layers']} layers"
        )
    for layer in keymap:
        if len(layer) != kb_json["keyboard"]["num_keys"]:
            raise ValueError(
                f"Expected default keymaps to have {kb_json['keyboard']['num_keys']} keys"
            )
