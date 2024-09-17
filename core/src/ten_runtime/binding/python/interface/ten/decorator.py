#
# Copyright © 2024 Agora
# This file is part of TEN Framework, an open source project.
# Licensed under the Apache License, Version 2.0, with certain conditions.
# Refer to the "LICENSE" file in the root directory for more information.
#
from libten_runtime_python import (
    _register_addon_as_extension,
    _register_addon_as_extension_group,
)


def register_addon_as_extension(name: str):
    return _register_addon_as_extension(name)


def register_addon_as_extension_group(name: str):
    return _register_addon_as_extension_group(name)
