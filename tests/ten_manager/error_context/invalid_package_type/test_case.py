#
# This file is part of the TEN Framework project.
# See https://github.com/TEN-framework/ten_framework/LICENSE for license
# information.
#
import os
import sys
from .common import cmd_exec


def test_invalid_package_type():
    base_path = os.path.dirname(os.path.abspath(__file__))
    root_dir = os.path.join(base_path, "../../../../")

    if sys.platform == "win32":
        os.environ["PATH"] = (
            os.path.join(root_dir, "ten_manager/lib") + ";" + os.getenv("PATH")
        )
        tman_bin = os.path.join(root_dir, "ten_manager/bin/tman.exe")
    else:
        tman_bin = os.path.join(root_dir, "ten_manager/bin/tman")

    app_dir = os.path.join(base_path, "test_app")
    origin_wd = os.getcwd()

    try:
        os.chdir(app_dir)

        config_file = os.path.join(
            root_dir,
            "tests/local_registry/config.json",
        )

        cmds = [
            tman_bin,
            f"--config-file={config_file}",
            "install",
        ]
        returncode, _ = cmd_exec.get_cmd_output(" ".join(cmds))

        assert returncode == 1
    finally:
        os.chdir(origin_wd)


if __name__ == "__main__":
    test_invalid_package_type()
