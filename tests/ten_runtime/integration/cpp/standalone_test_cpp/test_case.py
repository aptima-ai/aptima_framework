"""
Test standalone_test_cpp.
"""

import subprocess
import os
import pytest
from sys import stdout
from .common import build_config


# TODO(Wei): Remove this after solving the dlsym memory leakages.
@pytest.fixture(scope="function")
def manage_env_var_for_memory_issue_detection():
    original_ten_enable_memory_sanitizer = os.environ.get(
        "TEN_ENABLE_MEMORY_TRACKING"
    )
    original_asan_options = os.environ.get("ASAN_OPTIONS")

    # Set the environment variable before the test.
    os.environ["TEN_ENABLE_MEMORY_TRACKING"] = "false"
    os.environ["ASAN_OPTIONS"] = "detect_leaks=0"

    yield

    # Remove the environment variable after the test.
    if original_ten_enable_memory_sanitizer is not None:
        os.environ["TEN_ENABLE_MEMORY_TRACKING"] = (
            original_ten_enable_memory_sanitizer
        )
    else:
        del os.environ["TEN_ENABLE_MEMORY_TRACKING"]

    if original_asan_options is not None:
        os.environ["ASAN_OPTIONS"] = original_asan_options
    else:
        del os.environ["ASAN_OPTIONS"]


@pytest.mark.usefixtures("manage_env_var_for_memory_issue_detection")
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
        os.path.join(root_dir, "args.gn"),
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
