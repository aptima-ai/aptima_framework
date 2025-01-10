"""
Test websocket_server_nodejs.
"""

import os
import subprocess
import sys
from sys import stdout

from .common import build_config, build_pkg, http


def ws_request():
    """Send websocket request."""
    import websocket

    ws = websocket.create_connection("ws://localhost:8007")
    ws.send("Hello, World!")
    result = ws.recv()

    ws.close()
    return result


def test_websocket_server_nodejs():
    """Test client and app server."""
    base_path = os.path.dirname(os.path.abspath(__file__))
    root_dir = os.path.join(base_path, "../../../../../")

    my_env = os.environ.copy()

    source_pkg_name = "websocket_server_nodejs_app"
    app_root_path = os.path.join(base_path, source_pkg_name)
    app_language = "nodejs"

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

        # Compile typescript extensions.
        rc = build_pkg.build_nodejs_extensions(app_root_path)
        if rc != 0:
            assert False, "Failed to build TypeScript extensions."

    if sys.platform == "linux":
        if build_config_args.enable_sanitizer:
            libasan_path = os.path.join(
                base_path,
                "websocket_server_nodejs_app/aptima_packages/system/aptima_runtime/lib/libasan.so",
            )

            if os.path.exists(libasan_path):
                print("Using AddressSanitizer library.")
                my_env["LD_PRELOAD"] = libasan_path

    server_cmd = os.path.join(base_path, "websocket_server_nodejs_app/bin/start")

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

    is_started = http.is_app_started("127.0.0.1", 8002, 30)
    if not is_started:
        print("The websocket_server_nodejs is not started after 10 seconds.")

        server.kill()
        exit_code = server.wait()
        print("The exit code of websocket_server_nodejs: ", exit_code)

        assert exit_code == 0
        assert False

    try:
        print("Sending ws message to the app server.")
        resp = ws_request()
        assert resp == "Echo: Hello, World!"
    finally:
        is_stopped = http.stop_app("127.0.0.1", 8002, 30)
        if not is_stopped:
            print("The websocket_server_nodejs can not stop after 30 seconds.")
            server.kill()

        exit_code = server.wait()
        print("The exit code of websocket_server_nodejs: ", exit_code)

        assert exit_code == 0

        if build_config_args.aptima_enable_integration_tests_prebuilt is False:
            source_root_path = os.path.join(base_path, source_pkg_name)

            # Testing complete. If builds are only created during the testing
            # phase, we can clear the build results to save disk space.
            build_pkg.cleanup(source_root_path, app_root_path)
