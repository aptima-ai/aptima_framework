#
# This file is part of the TEN Framework project.
# See https://github.com/TEN-framework/ten_framework/LICENSE for license
# information.
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
        for module in glob(os.path.join(base_dir, "ten_packages/extension/*")):
            if os.path.isdir(module):
                module_name = os.path.basename(module)
                spec = importlib.util.find_spec(
                    "ten_packages.extension.{}".format(module_name)
                )
                if spec is not None:
                    _ = importlib.import_module(
                        "ten_packages.extension.{}".format(module_name)
                    )
                    print("imported module: {}".format(module_name))

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
