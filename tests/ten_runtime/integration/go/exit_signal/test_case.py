"""
Test exit_signal.
"""

import json
import os
import subprocess
import sys
from sys import stdout
import signal
import time
from .common import build_config, build_pkg


def start_app():
    """Test client and app server."""
    base_path = os.path.dirname(os.path.abspath(__file__))
    root_dir = os.path.join(base_path, "../../../../../")

    my_env = os.environ.copy()

    source_pkg_name = "exit_signal_app"
    app_root_path = os.path.join(base_path, source_pkg_name)
    app_language = "go"

    build_config_args = build_config.parse_build_config(
        os.path.join(root_dir, "tgn_args.txt"),
    )

    if build_config_args.ten_enable_integration_tests_prebuilt is False:
        print('Assembling and building package "{}".'.format(source_pkg_name))

        source_root_path = os.path.join(base_path, source_pkg_name)
        rc = build_pkg.prepare_and_build_app(
            build_config_args,
            root_dir,
            base_path,
            app_root_path,
            source_pkg_name,
            app_language,
        )
        if rc != 0:
            assert False, "Failed to build package."

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
        cwd=app_root_path,
    )
    tman_install_process.wait()

    if sys.platform == "win32":
        print("test_exit_signal doesn't support win32")
        assert False
    elif sys.platform == "darwin":
        # client depends on some libraries in the TEN app.
        my_env["DYLD_LIBRARY_PATH"] = os.path.join(
            base_path, "exit_signal_app/lib"
        )
    else:
        # client depends on some libraries in the TEN app.
        my_env["LD_LIBRARY_PATH"] = os.path.join(
            base_path, "exit_signal_app/lib"
        )

        if (
            build_config_args.enable_sanitizer
            and not build_config_args.is_clang
        ):
            libasan_path = os.path.join(
                base_path,
                "exit_signal_app/ten_packages/system/ten_runtime/lib/libasan.so",
            )
            if os.path.exists(libasan_path):
                my_env["LD_PRELOAD"] = libasan_path

    server_cmd = os.path.join(base_path, "exit_signal_app/bin/start")
    server = subprocess.Popen(
        server_cmd,
        stdout=stdout,
        stderr=subprocess.STDOUT,
        env=my_env,
        cwd=app_root_path,
    )

    # Wait for the extensions to start.
    time.sleep(3)

    return server


def check_if_extension_stops():
    base_dir = os.path.dirname(os.path.abspath(__file__))
    app_dir = os.path.join(base_dir, "exit_signal_app")
    exit_file = os.path.join(app_dir, "exit_signal.json")
    if not os.path.exists(exit_file):
        assert False

    with open(exit_file, "r") as f:
        data = json.load(f)
        assert "extension" in data and data["extension"] == "exit_signal"


def test_sigint():
    server = start_app()

    # Send Ctrl+C to stop the server.
    server.send_signal(signal.SIGINT)

    server_rc = server.wait()
    print("server: ", server_rc)
    assert server_rc == 0

    check_if_extension_stops()


def test_sigterm():
    server = start_app()

    # kill -15 <pid>
    server.send_signal(signal.SIGTERM)

    server_rc = server.wait()
    print("server: ", server_rc)
    assert server_rc == 0

    check_if_extension_stops()
