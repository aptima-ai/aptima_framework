"""
Test go_app_cythonize.
"""

import glob
import subprocess
import os
import sys
from sys import stdout
from .common import http


def http_request():
    return http.post(
        "http://127.0.0.1:8002/",
        {
            "_ten": {
                "name": "test",
            },
        },
    )


# compile pyx files in 'default python extension'.
def compile_pyx(app_root_path: str):
    extension_folder = os.path.join(
        app_root_path, "ten_packages/extension/default_extension_python"
    )

    # Search for .pyx files in extension_folder and all subdirectories.
    pyx_file_list = glob.glob(
        os.path.join(extension_folder, "**", "*.pyx"), recursive=True
    )
    if len(pyx_file_list) == 0:
        return

    # cp <app_root>/ten_packages/system/ten_runtime_python/tools/cython_compiler.py to
    # <app_root>/ten_packages/extension/default_extension_python
    import shutil

    script_file = "cython_compiler.py"

    script_path = os.path.join(
        app_root_path,
        "ten_packages/system/ten_runtime_python/tools",
        script_file,
    )
    target_file = os.path.join(extension_folder, script_file)
    shutil.copyfile(script_path, target_file)

    # Compile .pyx files.
    cython_compiler_cmd = [
        sys.executable,
        "cython_compiler.py",
    ]

    cython_compiler_process = subprocess.Popen(
        cython_compiler_cmd,
        stdout=stdout,
        stderr=subprocess.STDOUT,
        cwd=extension_folder,
    )
    cython_compiler_process.wait()

    # remove the script file.
    os.remove(target_file)

    # remove build/
    build_dir = os.path.join(
        app_root_path, "ten_packages/extension/default_extension_python/build"
    )
    if os.path.exists(build_dir):
        shutil.rmtree(build_dir)

    # Remove .pyx and .c files from extension_folder and all subdirectories
    pyx_file_list = glob.glob(
        os.path.join(extension_folder, "**", "*.pyx"), recursive=True
    )
    c_file_list = glob.glob(
        os.path.join(extension_folder, "**", "*.c"), recursive=True
    )
    for file in pyx_file_list:
        os.remove(file)
    for file in c_file_list:
        os.remove(file)


def test_go_app_cythonize():
    """Test client and app server."""
    base_path = os.path.dirname(os.path.abspath(__file__))
    root_dir = os.path.join(base_path, "../../../../../")

    # Create virtual environment.
    venv_dir = os.path.join(base_path, "venv")
    subprocess.run([sys.executable, "-m", "venv", venv_dir])

    my_env = os.environ.copy()

    # Set the required environment variables for the test.
    my_env["PYTHONMALLOC"] = "malloc"
    my_env["PYTHONDEVMODE"] = "1"

    # Launch virtual environment.
    my_env["VIRTUAL_ENV"] = venv_dir
    my_env["PATH"] = os.path.join(venv_dir, "bin") + os.pathsep + my_env["PATH"]

    if sys.platform == "win32":
        print("test_go_app_cythonize doesn't support win32")
        assert False
    elif sys.platform == "darwin":
        # client depends on some libraries in the TEN app.
        my_env["DYLD_LIBRARY_PATH"] = os.path.join(
            base_path, "go_app_cythonize_app/lib"
        )
    else:
        # client depends on some libraries in the TEN app.
        my_env["LD_LIBRARY_PATH"] = os.path.join(
            base_path, "go_app_cythonize_app/lib"
        )

    app_root_path = os.path.join(base_path, "go_app_cythonize_app")

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

    bootstrap_cmd = os.path.join(
        base_path, "go_app_cythonize_app/bin/bootstrap"
    )

    bootstrap_process = subprocess.Popen(
        bootstrap_cmd, stdout=stdout, stderr=subprocess.STDOUT, env=my_env
    )
    bootstrap_process.wait()

    compile_pyx(app_root_path)

    if sys.platform == "linux":
        if os.path.exists(os.path.join(base_path, "use_asan_lib_marker")):
            libasan_path = os.path.join(
                base_path,
                "go_app_cythonize_app/ten_packages/system/ten_runtime/lib/libasan.so",
            )

            if os.path.exists(libasan_path):
                my_env["LD_PRELOAD"] = libasan_path

    server_cmd = os.path.join(base_path, "go_app_cythonize_app/bin/start")

    server = subprocess.Popen(
        server_cmd,
        stdout=stdout,
        stderr=subprocess.STDOUT,
        env=my_env,
        cwd=app_root_path,
    )

    is_started = http.is_app_started("127.0.0.1", 8002, 30)
    if not is_started:
        print("The go_app_cythonize is not started after 30 seconds.")

        server.kill()
        exit_code = server.wait()
        print("The exit code of go_app_cythonize: ", exit_code)

        assert exit_code == 0
        assert 0

        return

    try:
        resp = http_request()
        assert resp != 500
        print(resp)

    finally:
        is_stopped = http.stop_app("127.0.0.1", 8002, 30)
        if not is_stopped:
            print("The go_app_cythonize can not stop after 30 seconds.")
            server.kill()

        exit_code = server.wait()
        print("The exit code of go_app_cythonize: ", exit_code)

        assert exit_code == 0
