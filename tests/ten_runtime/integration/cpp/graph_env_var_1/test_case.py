"""
Test graph_env_var_1_app.
"""

import subprocess
import os
import sys
from sys import stdout
from .common import msgpack


def test_graph_env_var_1_app():
    """Test client and app server."""
    base_path = os.path.dirname(os.path.abspath(__file__))
    root_dir = os.path.join(base_path, "../../../../../")

    my_env = os.environ.copy()

    app_root_path = os.path.join(base_path, "graph_env_var_1_app")

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
                base_path,
                "graph_env_var_1_app/ten_packages/system/ten_runtime/lib",
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
        # client depends on some libraries in the TEN app.
        my_env["DYLD_LIBRARY_PATH"] = os.path.join(
            base_path,
            "graph_env_var_1_app/ten_packages/system/ten_runtime/lib",
        )
        server_cmd = os.path.join(
            base_path, "graph_env_var_1_app/bin/graph_env_var_1_app_source"
        )
        client_cmd = os.path.join(base_path, "graph_env_var_1_app_client")
    else:
        # client depends on some libraries in the TEN app.
        my_env["LD_LIBRARY_PATH"] = os.path.join(
            base_path,
            "graph_env_var_1_app/ten_packages/system/ten_runtime/lib",
        )
        server_cmd = os.path.join(
            base_path, "graph_env_var_1_app/bin/graph_env_var_1_app_source"
        )
        client_cmd = os.path.join(base_path, "graph_env_var_1_app_client")

        if os.path.exists(os.path.join(base_path, "use_asan_lib_marker")):
            libasan_path = os.path.join(
                base_path,
                "graph_env_var_1_app/ten_packages/system/ten_runtime/lib/libasan.so",
            )
            if os.path.exists(libasan_path):
                my_env["LD_PRELOAD"] = libasan_path

    my_env["TEST_ENV_VAR"] = "set_from_real_env_var"

    server = subprocess.Popen(
        server_cmd, stdout=stdout, stderr=subprocess.STDOUT, env=my_env
    )

    is_started, sock = msgpack.is_app_started("127.0.0.1", 8001, 10)
    if not is_started:
        print("The graph_env_var_1_app is not started after 30 seconds.")

        server.kill()
        exit_code = server.wait()
        print("The exit code of graph_env_var_1_app: ", exit_code)

        assert exit_code == 0
        assert 0

        return

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
    assert server_rc == 0
    assert client_rc == 0
