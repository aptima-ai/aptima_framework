#
# Copyright © 2024 Agora
# This file is part of TEN Framework, an open source project.
# Licensed under the Apache License, Version 2.0, with certain conditions.
# Refer to the "LICENSE" file in the root directory for more information.
#
import os
import sys
from typing import Type
from .addon import Addon
from .addon_manager import _AddonManager
from libten_runtime_python import (
    _register_addon_as_extension,
    _register_addon_as_extension_v2,
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


def register_addon_as_extension_v2(name: str, base_dir: str | None = None):
    """
    Decorator to register a class as an addon extension and create a special
    registration function required by the Addon loader.

    Args:
        name (str): The name of the addon extension.
        base_dir (str, optional): The base directory of the addon. Defaults to
            None.

    Returns:
        Callable: The decorator function.
    """

    def decorator(cls: Type[Addon]) -> Type[Addon]:
        # Resolve base_dir.
        if base_dir is None:
            try:
                # Attempt to get the caller's file path using sys._getframe()
                caller_frame = sys._getframe(1)
                resolved_base_dir = os.path.dirname(
                    caller_frame.f_code.co_filename
                )
            except (AttributeError, ValueError):
                # Fallback in case sys._getframe() is not available or fails.
                # Example: in Cython or restricted environments.
                resolved_base_dir = None
        else:
            # If base_dir is provided, ensure it's the directory name
            resolved_base_dir = os.path.dirname(base_dir)

        # Define the registration function that will be called by the Addon
        # loader.
        def registration_func(register_ctx):
            """
            Registration function injected into the module to handle addon
            registration.

            Args:
                register_ctx: An opaque parameter provided by the Addon loader.
            """
            # Instantiate the addon class.
            instance = cls()

            try:
                _register_addon_as_extension_v2(
                    name, resolved_base_dir, instance, register_ctx
                )
                print(
                    f"Called '_register_addon_as_extension' for addon '{name}'"
                )
            except Exception as e:
                print(f"Failed to register addon '{name}': {e}")

        # Define the registration function name based on the addon name.
        _AddonManager._set_registration_func(name, registration_func)

        # Return the original class without modification.
        return cls

    return decorator
