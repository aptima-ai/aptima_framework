"""
Test restful_http_app.
"""

import subprocess
import os
import sys
from sys import stdout
from .common import http


def run_test_post():
    resp = http.post(
        "http://127.0.0.1:8001/",
        {"_ten": {"name": "hello_world"}},
    )

    assert resp == '"hello_world, too"'


def test_restful_http_app():
    """Test app server."""
    base_path = os.path.dirname(os.path.abspath(__file__))
    root_dir = os.path.join(base_path, "../../../../../")

    my_env = os.environ.copy()

    if sys.platform == "win32":
        my_env["PATH"] = (
            os.path.join(
                base_path,
                "restful_http_app/ten_packages/system/ten_runtime/lib",
            )
            + ";"
            + my_env["PATH"]
        )
        server_cmd = os.path.join(
            base_path, "restful_http_app/bin/restful_app_source.exe"
        )
    elif sys.platform == "darwin":
        server_cmd = os.path.join(
            base_path, "restful_http_app/bin/restful_app_source"
        )
    else:
        server_cmd = os.path.join(
            base_path, "restful_http_app/bin/restful_app_source"
        )

    app_root_path = os.path.join(base_path, "restful_http_app")

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

    server = subprocess.Popen(
        server_cmd,
        stdout=stdout,
        stderr=subprocess.STDOUT,
        env=my_env,
        cwd=app_root_path,
    )

    is_started = http.is_app_started("127.0.0.1", 8001, 30)
    if not is_started:
        print("The restful_http_app is not started after 30 seconds.")

        server.kill()
        exit_code = server.wait()
        print("The exit code of restful_http_app: ", exit_code)

        assert exit_code == 0
        assert 0

        return

    try:
        run_test_post()

    finally:
        is_stopped = http.stop_app("127.0.0.1", 8001, 30)

        if not is_stopped:
            print("The restful_http_app can not stop after 30 seconds.")
            server.kill()

        exit_code = server.wait()
        print("The exit code of restful_http_app: ", exit_code)

        assert exit_code == 0
