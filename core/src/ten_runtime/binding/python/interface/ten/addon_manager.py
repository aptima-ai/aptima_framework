#
# Copyright © 2024 Agora
# This file is part of TEN Framework, an open source project.
# Licensed under the Apache License, Version 2.0, with certain conditions.
# Refer to the "LICENSE" file in the root directory for more information.
#
import json
import os
import sys
import importlib.util
from glob import glob
from typing import Callable, Any, Dict, Type
from .addon import Addon
from libten_runtime_python import (
    _register_addon_as_extension,
    _register_addon_as_extension_v2,
)


class _AddonManager:
    # Use the simple approach below, similar to a global array, to detect
    # whether a Python module provides the registration function required by the
    # TEN runtime. This avoids using `setattr` on the module, which may not be
    # supported in advanced environments like Cython. The global array method
    # is simple enough that it should work in all environments.
    _registration_registry: Dict[str, Callable[[Any], None]] = {}

    @classmethod
    def load_all_addons(cls, register_ctx: object):
        base_dir = cls._find_app_base_dir()

        # Read manifest.json under base_dir.
        manifest_path = os.path.join(base_dir, "manifest.json")
        if not os.path.isfile(manifest_path):
            raise FileNotFoundError("manifest.json not found in base_dir")

        with open(manifest_path, "r") as f:
            manifest = json.load(f)

        # Note: The logic for loading extensions based on the `dependencies`
        # specified in the app's `manifest.json` is currently implemented
        # separately in both C and Python where addons need to be loaded. Since
        # the logic is fairly simple, a standalone implementation is directly
        # written at each required location. In the future, this could be
        # consolidated into a unified implementation in C, which could then be
        # reused across multiple languages. However, this would require handling
        # cross-language information exchange, which may not necessarily be
        # cost-effective.

        # Collect names of extensions from dependencies.
        extension_names = []
        dependencies = manifest.get("dependencies", [])
        for dep in dependencies:
            if dep.get("type") == "extension":
                extension_names.append(dep.get("name"))

        for module in glob(os.path.join(base_dir, "ten_packages/extension/*")):
            if os.path.isdir(module):
                module_name = os.path.basename(module)

                if module_name in extension_names:
                    cls._load_module(
                        module_full_name=(
                            f"ten_packages.extension.{module_name}"
                        ),
                        module_name=module_name,
                        register_ctx=register_ctx,
                    )
                else:
                    print(f"Skipping module: {module_name}")

    @classmethod
    def _load_module(
        cls,
        module_full_name: str,
        module_name: str,
        register_ctx: object,
    ):
        """
        Helper method to load a module, check for the special function,
        invoke it, and unload the module if the special function is missing.
        """
        try:
            spec = importlib.util.find_spec(module_full_name)
            if spec is None:
                raise ImportError(f"Cannot find module: {module_full_name}")

            _ = importlib.import_module(module_full_name)
            print(f"Imported module: {module_name}")

            # Retrieve the registration function from the global registry
            registration_func_name = _AddonManager._get_registration_func_name(
                module_name
            )

            registration_func = _AddonManager._get_registration_func(
                module_name
            )

            if registration_func:
                try:
                    registration_func(register_ctx)
                    print(f"Successfully registered addon '{module_name}'")
                except Exception as e:
                    print(
                        (
                            "Error during registration of addon "
                            f"'{module_name}': {e}"
                        )
                    )
                finally:
                    # Remove the registration function from the global registry.
                    if (
                        registration_func_name
                        in _AddonManager._registration_registry
                    ):
                        del _AddonManager._registration_registry[
                            registration_func_name
                        ]
                        print(
                            (
                                "Removed registration function for addon "
                                f"'{registration_func_name}'"
                            )
                        )
            else:
                print(f"No {registration_func_name} found in {module_name}")

        except ImportError as e:
            print(f"Error importing module {module_name}: {e}")

    @staticmethod
    def _get_registration_func_name(addon_name: str) -> str:
        return f"____ten_addon_{addon_name}_register____"

    @staticmethod
    def _get_registration_func(addon_name: str) -> Callable[[Any], None] | None:
        return _AddonManager._registration_registry.get(
            _AddonManager._get_registration_func_name(addon_name)
        )

    @staticmethod
    def _set_registration_func(
        addon_name: str,
        registration_func: Callable[[Any], None],
    ) -> None:
        registration_func_name = _AddonManager._get_registration_func_name(
            addon_name
        )

        print(
            (
                f"Injected registration function '{registration_func_name}' "
                "into module '{module.__name__}'"
            )
        )

        _AddonManager._registration_registry[registration_func_name] = (
            registration_func
        )

    @staticmethod
    def _find_app_base_dir():
        current_dir = os.path.dirname(__file__)

        while current_dir != os.path.dirname(
            current_dir
        ):  # Stop at the root directory.
            manifest_path = os.path.join(current_dir, "manifest.json")
            if os.path.isfile(manifest_path):
                with open(manifest_path, "r") as manifest_file:
                    try:
                        manifest_data = json.load(manifest_file)
                        if manifest_data.get("type") == "app":
                            return current_dir
                    except json.JSONDecodeError:
                        pass
            current_dir = os.path.dirname(current_dir)

        raise FileNotFoundError(
            "App base directory with a valid manifest.json not found."
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
