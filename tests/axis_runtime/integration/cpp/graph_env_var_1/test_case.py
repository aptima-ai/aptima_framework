"""
Test graph_env_var_1_app.
"""

import os
import subprocess
import sys
from sys import stdout

from .common import build_config, build_pkg, msgpack


def test_graph_env_var_1_app():
    """Test client and app server."""
    base_path = os.path.dirname(os.path.abspath(__file__))
    root_dir = os.path.join(base_path, "../../../../../")

    my_env = os.environ.copy()

    app_root_path = os.path.join(base_path, "graph_env_var_1_app")
    source_pkg_name = "graph_env_var_1_app_source"
    app_language = "cpp"

    build_config_args = build_config.parse_build_config(
        os.path.join(root_dir, "tgn_args.txt"),
    )

    if build_config_args.aptima_enable_integration_tests_prebuilt is False:
        print('Assembling and building package "{}".'.format(source_pkg_name))

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
        os.path.join(root_dir, "aptima_manager/bin/tman"),
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
    return_code = tman_install_process.returncode
    if return_code != 0:
        assert False, "Failed to install package."

    if sys.platform == "win32":
        my_env["PATH"] = (
            os.path.join(
                base_path,
                "graph_env_var_1_app/aptima_packages/system/aptima_runtime/lib",
            )
            + ";"
            + my_env["PATH"]
        )
        server_cmd = os.path.join(
            base_path,
            "graph_env_var_1_app/bin/graph_env_var_1_app_source.exe",
        )
        client_cmd = os.path.join(base_path, "graph_env_var_1_app_client.exe")
    elif sys.platform == "darwin":
        # client depends on some libraries in the APTIMA app.
        my_env["DYLD_LIBRARY_PATH"] = os.path.join(
            base_path,
            "graph_env_var_1_app/aptima_packages/system/aptima_runtime/lib",
        )
        server_cmd = os.path.join(
            base_path, "graph_env_var_1_app/bin/graph_env_var_1_app_source"
        )
        client_cmd = os.path.join(base_path, "graph_env_var_1_app_client")
    else:
        # client depends on some libraries in the APTIMA app.
        my_env["LD_LIBRARY_PATH"] = os.path.join(
            base_path,
            "graph_env_var_1_app/aptima_packages/system/aptima_runtime/lib",
        )
        server_cmd = os.path.join(
            base_path, "graph_env_var_1_app/bin/graph_env_var_1_app_source"
        )
        client_cmd = os.path.join(base_path, "graph_env_var_1_app_client")

        if (
            build_config_args.enable_sanitizer
            and not build_config_args.is_clang
        ):
            libasan_path = os.path.join(
                base_path,
                "graph_env_var_1_app/aptima_packages/system/aptima_runtime/lib/libasan.so",
            )
            if os.path.exists(libasan_path):
                print("Using AddressSanitizer library.")
                my_env["LD_PRELOAD"] = libasan_path

    my_env["TEST_ENV_VAR"] = "set_from_real_env_var"

    if not os.path.isfile(server_cmd):
        print(f"Server command '{server_cmd}' does not exist.")
        assert False

    server = subprocess.Popen(
        server_cmd,
        stdout=stdout,
        stderr=subprocess.STDOUT,
        env=my_env,
        cwd=app_root_path,
    )

    is_started, sock = msgpack.is_app_started("127.0.0.1", 8001, 10)
    if not is_started:
        print("The graph_env_var_1_app is not started after 10 seconds.")

        server.kill()
        server_rc = server.wait()

        print("The exit code of graph_env_var_1_app: ", server_rc)

        assert server_rc == 0, f"Server exited with code {server_rc}"
        assert False

    client = subprocess.Popen(
        client_cmd, stdout=stdout, stderr=subprocess.STDOUT, env=my_env
    )

    # The client will quit after the test is completed.
    client_rc = client.wait()
    if client_rc != 0:
        server.kill()

    # We cannot shutdown the socket before the client is closed, due to it will
    # trigger the GC of the app server.
    sock.close()

    server_rc = server.wait()
    print("server: ", server_rc)
    print("client: ", client_rc)
    assert server_rc == 0, f"Server exited with code {server_rc}"
    assert client_rc == 0, f"Client exited with code {client_rc}"

    if build_config_args.aptima_enable_integration_tests_prebuilt is False:
        source_root_path = os.path.join(base_path, source_pkg_name)

        # Testing complete. If builds are only created during the testing phase,
        # we can clear the build results to save disk space.
        build_pkg.cleanup(source_root_path, app_root_path)
