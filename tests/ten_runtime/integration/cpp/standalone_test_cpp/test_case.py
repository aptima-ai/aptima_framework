"""
Test standalone_test_cpp.
"""

import subprocess
import os
from sys import stdout
from .common import build_config


def test_standalone_test_cpp():
    base_path = os.path.dirname(os.path.abspath(__file__))
    root_dir = os.path.join(base_path, "../../../../../")

    my_env = os.environ.copy()

    extension_root_path = os.path.join(base_path, "default_extension_cpp")

    # Step 1:
    #
    # Standalone testing involves the use of ten_runtime, so use tman install to
    # install the ten_runtime system package.
    tman_install_cmd = [
        os.path.join(root_dir, "ten_manager/bin/tman"),
        "--config-file",
        os.path.join(root_dir, "tests/local_registry/config.json"),
        "install",
    ]

    tman_install_process = subprocess.Popen(
        tman_install_cmd,
        stdout=stdout,
        stderr=subprocess.STDOUT,
        env=my_env,
        cwd=extension_root_path,
    )
    tman_install_rc = tman_install_process.wait()
    assert tman_install_rc == 0

    build_config_args = build_config.parse_build_config(
        os.path.join(root_dir, "tgn_args.txt"),
    )

    # Step 2:
    #
    # Execute tgn gen to generate the build files.
    if build_config_args.target_os == "win":
        tgn_path = os.getenv("tgn")
    else:
        tgn_path = "tgn"

    tgn_gen_cmd = [
        tgn_path,
        "gen",
        build_config_args.target_os,
        build_config_args.target_cpu,
        build_config_args.target_build,
        "--",
        "ten_enable_standalone_test=true",
    ]

    tgn_gen_process = subprocess.Popen(
        tgn_gen_cmd,
        stdout=stdout,
        stderr=subprocess.STDOUT,
        env=my_env,
        cwd=extension_root_path,
    )
    tgn_gen_rc = tgn_gen_process.wait()
    assert tgn_gen_rc == 0

    # Step 3:
    #
    # Execute tgn build to build the extension and its test cases.
    tgn_build_cmd = [
        tgn_path,
        "build",
        build_config_args.target_os,
        build_config_args.target_cpu,
        build_config_args.target_build,
    ]

    tgn_build_process = subprocess.Popen(
        tgn_build_cmd,
        stdout=stdout,
        stderr=subprocess.STDOUT,
        env=my_env,
        cwd=extension_root_path,
    )
    tgn_build_rc = tgn_build_process.wait()
    assert tgn_build_rc == 0

    # Step 4:
    #
    # Run standalone test cases.
    tester_cmd = [
        os.path.join(
            extension_root_path,
            (
                f"out/{build_config_args.target_os}/"
                f"{build_config_args.target_cpu}/"
                "tests/default_extension_cpp_test"
                + (".exe" if build_config_args.target_os == "win" else "")
            ),
        ),
    ]

    my_env["TEN_ENABLE_MEMORY_TRACKING"] = "true" + ";" + my_env["PATH"]

    if build_config_args.target_os == "win":
        my_env["PATH"] = (
            os.path.join(
                extension_root_path,
                (
                    f"out/{build_config_args.target_os}/"
                    f"{build_config_args.target_cpu}/"
                    "ten_packages/system/ten_runtime/lib"
                ),
            )
            + ";"
            + my_env["PATH"]
        )

    tester_process = subprocess.Popen(
        tester_cmd,
        stdout=stdout,
        stderr=subprocess.STDOUT,
        env=my_env,
        cwd=extension_root_path,
    )
    tester_rc = tester_process.wait()
    assert tester_rc == 0
