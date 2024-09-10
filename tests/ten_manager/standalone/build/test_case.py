"""
Test standalone build.
"""

import json
import os
import sys


def check_if_template_mode_works(base_path: str):
    manifest_file = os.path.join(base_path, "manifest.json")
    if not os.path.exists(manifest_file):
        assert False, "The manifest file is not found."

    with open(manifest_file, "r") as f:
        manifest_json = json.load(f)
        if manifest_json["name"] != "build_extension":
            assert False, "The package name is not build_extension."


def check_if_standalone_install_works(base_path: str):
    deps_dir = os.path.join(base_path, ".ten/app")
    if not os.path.exists(deps_dir):
        assert False, "The .ten/app is not found."

    runtime_manifest = os.path.join(
        deps_dir, "ten_packages/system/ten_runtime/manifest.json"
    )
    if not os.path.exists(runtime_manifest):
        assert False, "The manifest.json of ten_runtime is not found in deps."

    with open(runtime_manifest, "r") as f:
        manifest_json = json.load(f)
        if manifest_json["name"] != "ten_runtime":
            assert False, "The package name is not ten_runtime."


def check_if_standalone_build_works(base_path: str):
    out_dir = os.path.join(base_path, "out", sys.platform)
    if not os.path.exists(out_dir):
        assert False, "The out directory is not found."

    dirs = os.listdir(out_dir)
    assert len(dirs) == 1

    out_dir = os.path.join(
        out_dir, dirs[0], "ten_packages/extension/build_extension/lib"
    )
    if not os.path.exists(out_dir):
        assert False, "The out directory is not found."

    found_output_lib = False
    for f in os.listdir(out_dir):
        if "build_extension" in f:
            found_output_lib = True
            break

    assert found_output_lib, "The output lib is not found."


def test_standalone_build():
    base_path = os.path.dirname(os.path.abspath(__file__))
    extension = os.path.join(base_path, "build_extension")
    if not os.path.exists(extension):
        assert False, "The extension is not found."

    check_if_template_mode_works(extension)
    check_if_standalone_install_works(extension)
    check_if_standalone_build_works(extension)
