#
# This file is part of the TEN Framework project.
# See https://github.com/TEN-framework/ten_framework/LICENSE for license
# information.
#
import argparse
import os
import sys
from build.scripts import cmd_exec


class ArgumentInfo(argparse.Namespace):
    def __init__(self):
        super().__init__()
        self.python_version: str
        self.target_os: str
        self.config_type: str
        self.log_level: int


if __name__ == "__main__":
    parser = argparse.ArgumentParser()

    parser.add_argument("--python-version", type=str, required=True)
    parser.add_argument("--target-os", type=str, required=True)
    parser.add_argument("--config-type", type=str, required=True)
    parser.add_argument("--log-level", type=int, required=True)

    arg_info = ArgumentInfo()
    args = parser.parse_args(namespace=arg_info)

    try:
        if args.target_os == "win":
            py_path = os.environ.get("PYTHON3_PATH")
            if py_path is None:
                raise SystemError("PYTHON3_PATH is not provided")
            else:
                if args.config_type == "cflags":
                    print('/I"' + os.path.join(py_path, "include") + '"')
                elif args.config_type == "ldflags":
                    print('/LIBPATH:"' + os.path.join(py_path, "libs" + '"'))
                elif args.config_type == "libs":
                    print('/Libs:"' + os.path.join(py_path, "libs" + '"'))
                else:
                    raise Exception(f"Unknown option: {args.config_type}")
        else:
            if args.config_type == "cflags":
                cmd = [
                    f"python{args.python_version}-config",
                    "--embed",
                    "--cflags",
                ]
            elif args.config_type == "ldflags":
                cmd = [
                    f"python{args.python_version}-config",
                    "--embed",
                    "--ldflags",
                ]
            elif args.config_type == "libs":
                cmd = [
                    f"python{args.python_version}-config",
                    "--embed",
                    "--libs",
                    "| sed 's/-l//g'",  # remove -l prefix
                ]
            else:
                raise Exception(f"Unknown option: {args.config_type}")

            returncode, output_text = cmd_exec.run_cmd(cmd, args.log_level)
            if returncode:
                raise Exception("Failed to run python-config.")

            outputs = output_text.split()
            for output in outputs:
                print(output)

    except Exception as exc:
        print(exc)

    finally:
        sys.exit(-1 if returncode != 0 else 0)
