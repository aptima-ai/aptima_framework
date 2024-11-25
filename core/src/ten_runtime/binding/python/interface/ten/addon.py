#
# Copyright © 2024 Agora
# This file is part of TEN Framework, an open source project.
# Licensed under the Apache License, Version 2.0, with certain conditions.
# Refer to the "LICENSE" file in the root directory for more information.
#
import json
import os
import importlib.util
from glob import glob
from libten_runtime_python import _Addon
from .ten_env import TenEnv


class Addon(_Addon):
    @classmethod
    def _load_all(cls):
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
                    # Proceed to load the module.
                    spec = importlib.util.find_spec(
                        "ten_packages.extension.{}".format(module_name)
                    )
                    if spec is not None:
                        _ = importlib.import_module(
                            "ten_packages.extension.{}".format(module_name)
                        )
                        print("imported module: {}".format(module_name))
                else:
                    print("Skipping module: {}".format(module_name))

    @classmethod
    def _load_from_path(cls, path):
        module_name = os.path.basename(path)
        spec = importlib.util.find_spec(module_name)
        if spec is not None:
            mod = importlib.import_module(module_name)
            print(f"Imported module: {module_name}")
            return mod
        else:
            raise ImportError(f"Cannot find module: {module_name}")

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

    def on_init(self, ten_env: TenEnv) -> None:
        ten_env.on_init_done()

    def on_deinit(self, ten_env: TenEnv) -> None:
        ten_env.on_deinit_done()

    def on_create_instance(
        self, ten_env: TenEnv, name: str, context
    ) -> None: ...
