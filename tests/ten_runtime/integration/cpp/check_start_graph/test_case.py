"""
Test check_start_graph.
"""

import subprocess
import os
import sys
from sys import stdout
from .common import build_config, build_pkg


def test_check_start_graph():
    """Test client and app server."""
    base_path = os.path.dirname(os.path.abspath(__file__))
    root_dir = os.path.join(base_path, "../../../../../")

    my_env = os.environ.copy()

    app_root_path = os.path.join(base_path, "check_start_graph_app")
    source_pkg_name = "check_start_graph_source"
    app_language = "cpp"

    build_config_args = build_config.parse_build_config(
        os.path.join(root_dir, "ten_args.gn"),
    )

    if build_config_args.enable_prebuilt is False:
        print("Build package first.")

        source_root_path = os.path.join(base_path, source_pkg_name)
        rc = build_pkg.build(
            build_config_args,
            source_root_path,
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
        my_env["PATH"] = (
            os.path.join(
                app_root_path,
                "ten_packages/system/ten_runtime/lib",
            )
            + ";"
            + my_env["PATH"]
        )
        server_cmd = os.path.join(
            app_root_path, "bin/check_start_graph_source.exe"
        )
    elif sys.platform == "darwin":
        server_cmd = os.path.join(app_root_path, "bin/check_start_graph_source")
    else:
        server_cmd = os.path.join(app_root_path, "bin/check_start_graph_source")

        if (
            build_config_args.enable_sanitizer
            and not build_config_args.is_clang
        ):
            libasan_path = os.path.join(
                app_root_path,
                "ten_packages/system/ten_runtime/lib/libasan.so",
            )
            if os.path.exists(libasan_path):
                my_env["LD_PRELOAD"] = libasan_path

    server = subprocess.Popen(
        server_cmd,
        stdout=stdout,
        stderr=subprocess.STDOUT,
        env=my_env,
        cwd=app_root_path,
    )

    server_rc = server.wait(10)
    if server_rc != 0:
        server.kill()

    print("server: ", server_rc)
    assert server_rc == 0
