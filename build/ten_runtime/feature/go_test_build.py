#
# Copyright © 2024 Agora
# This file is part of TEN Framework, an open source project.
# Licensed under the Apache License, Version 2.0, with certain conditions.
# Refer to the "LICENSE" file in the root directory for more information.
#
import argparse
import os
import subprocess
import sys


class ArgumentInfo(argparse.Namespace):
    def __init__(self):
        self.source_dir: str
        self.system_go_dir: str
        self.output: str
        self.is_clang: bool
        self.enable_sanitizer: bool


def run_cmd(cmd: list[str] | str, cwd: str, envs: dict[str, str]) -> int:
    # print("Build go test: ", cmd)

    my_cmd = cmd
    set_shell = True

    if isinstance(cmd, list):
        if sys.platform == "win32":
            if cmd[0] != "cmd":
                my_cmd = ["cmd", "/c"] + cmd
        else:
            set_shell = False

    if isinstance(cmd, str):
        if sys.platform == "win32" and cmd[:3] != "cmd":
            my_cmd = "cmd /c " + cmd

    child = subprocess.Popen(
        my_cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        universal_newlines=True,
        shell=set_shell,
        bufsize=0,
        cwd=cwd,
        env=envs,
    )

    while child.poll() is None:
        line = ""
        if child.stdout:
            try:
                line = child.stdout.readline()
            except UnicodeDecodeError:
                line = child.stdout.readline().encode("gbk")

        if line != "":
            sys.stdout.flush()
            sys.stdout.write("{}\n".format(line.rstrip()))
            sys.stdout.flush()

    return child.returncode


if __name__ == "__main__":
    parser = argparse.ArgumentParser()

    parser.add_argument("--source-dir", type=str, required=True)
    parser.add_argument("--system-go-dir", type=str, required=True)
    parser.add_argument("--output", type=str, required=True)
    parser.add_argument("--is-clang", action=argparse.BooleanOptionalAction)
    parser.add_argument(
        "--enable-sanitizer", action=argparse.BooleanOptionalAction
    )

    arg_info = ArgumentInfo()
    args = parser.parse_args(namespace=arg_info)

    returncode = 0

    envs = os.environ.copy()

    if sys.platform == "linux":
        envs["CGO_LDFLAGS"] = (
            "-L{path} -lten_runtime_go -Wl,-rpath={path}".format(
                path=args.system_go_dir
            )
        )
    else:
        envs["CGO_LDFLAGS"] = (
            "-L{path} -lten_runtime_go -Wl,-rpath,{path}".format(
                path=args.system_go_dir
            )
        )

    if args.is_clang:
        envs["CC"] = "clang"
        envs["CXX"] = "clang++"
    else:
        envs["CC"] = "gcc"
        envs["CXX"] = "g++"

    cmd = ["go", "test", "-c", "-o", args.output]

    if args.enable_sanitizer:
        cmd += ["-asan"]

    try:
        returncode = run_cmd(cmd, args.source_dir, envs)
        if returncode != 0:
            raise Exception("Failed to build go test.")

    except Exception as exc:
        returncode = 1
        print("Failed to build go test:")
        print(f"\t{exc}")

    finally:
        sys.exit(returncode)
