#
# This file is part of the TEN Framework project.
# See https://github.com/TEN-framework/ten_framework/LICENSE for license
# information.
#
from libten_runtime_python import (
    _register_addon_as_extension,
    _register_addon_as_extension_group,
)


def register_addon_as_extension(name: str):
    return _register_addon_as_extension(name)


def register_addon_as_extension_group(name: str):
    return _register_addon_as_extension_group(name)
