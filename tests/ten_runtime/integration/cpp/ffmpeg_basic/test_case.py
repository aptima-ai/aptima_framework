"""
Test ffmpeg_basic_app.
"""

import subprocess
import os
import sys
from . import video_cmp
from sys import stdout


def test_ffmpeg_basic_app():
    """Test client and app server."""
    base_path = os.path.dirname(os.path.abspath(__file__))
    root_dir = os.path.join(base_path, "../../../../../")

    my_env = os.environ.copy()

    app_root_path = os.path.join(base_path, "ffmpeg_basic_app")

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
                "ffmpeg_basic_app/ten_packages/system/ten_runtime/lib",
            )
            + ";"
            + my_env["PATH"]
        )
        server_cmd = "bin/ffmpeg_basic_app_source.exe"
    elif sys.platform == "darwin":
        # client depends on some libraries in the TEN app.
        my_env["DYLD_LIBRARY_PATH"] = os.path.join(
            base_path, "ffmpeg_basic_app/ten_packages/system/ten_runtime/lib"
        )
        server_cmd = "bin/ffmpeg_basic_app_source"
    else:
        # client depends on some libraries in the TEN app.
        my_env["LD_LIBRARY_PATH"] = os.path.join(
            base_path, "ffmpeg_basic_app/ten_packages/system/ten_runtime/lib"
        )
        server_cmd = "bin/ffmpeg_basic_app_source"

        if os.path.exists(os.path.join(base_path, "use_asan_lib_marker")):
            libasan_path = os.path.join(
                base_path,
                "ffmpeg_basic_app/ten_packages/system/ten_runtime/lib/libasan.so",
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

    server_rc = server.wait()
    print("server: ", server_rc)

    assert server_rc == 0

    cmp_rc = video_cmp.compareVideo(
        os.path.join(
            app_root_path, "ten_packages/extension/ffmpeg_demuxer/res/test.mp4"
        ),
        os.path.join(
            app_root_path, "ten_packages/extension/ffmpeg_muxer/test.mp4"
        ),
    )
    assert cmp_rc
    # python cv2 would set LD_LIBRARY_PATH to 'cwd', and this will cause the
    # TEN app of the subsequent integration test cases to use the 'libten_runtime.so'
    # under 'out/<OS>/<CPU>/, rather than the one under '<TEN_app>/lib/'. This
    # is not what TEN runtime expects, so we unset 'LD_LIBRARY_PATH' to prevent
    # from this happening.
    #
    # Refer to ~/.local/lib/python3.10/site-packages/cv2/__init__.py after
    # 'cv2' has been installed.
    #
    # ...
    # os.environ['LD_LIBRARY_PATH'] = ':'.join(l_vars['BINARIES_PATHS']) + ':'
    # + os.environ.get('LD_LIBRARY_PATH', '')
    #                   ^^^^^^^^^^^^^^^   ^^ default to empty, means 'cwd'
    # ...
    try:
        del os.environ["LD_LIBRARY_PATH"]
    except Exception as e:
        # Maybe 'LD_LIBRARY_PATH' has been unset.
        print(e)
