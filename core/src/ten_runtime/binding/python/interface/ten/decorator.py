#
# Copyright © 2024 Agora
# This file is part of TEN Framework, an open source project.
# Licensed under the Apache License, Version 2.0, with certain conditions.
# Refer to the "LICENSE" file in the root directory for more information.
#
import os
import sys
from libten_runtime_python import (
    _register_addon_as_extension,
)


def register_addon_as_extension(name: str, base_dir: str | None = None):
    if base_dir is None:
        try:
            # Attempt to get the caller's file path using sys._getframe()
            caller_frame = sys._getframe(1)
            base_dir = os.path.dirname(caller_frame.f_code.co_filename)
        except (AttributeError, ValueError):
            # Fallback in case sys._getframe() is not available or fails.
            # Ex: in cython.
            base_dir = None

    # If base_dir is not None, convert it to its directory name.
    if base_dir is not None:
        base_dir = os.path.dirname(base_dir)

    return _register_addon_as_extension(name, base_dir)
