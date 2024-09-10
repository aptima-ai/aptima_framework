#
# This file is part of the TEN Framework project.
# See https://github.com/TEN-framework/ten_framework/LICENSE for license
# information.
#
import argparse
import sys
import os
from build.scripts import cmd_exec, timestamp_proxy, write_depfile


class ArgumentInfo(argparse.Namespace):
    def __init__(self):
        self.tman_path: str
        self.pkg_src_root_dir: str
        self.tg_timestamp_proxy_file: str
        self.src_pkg: str
        self.build_type: str
        self.config_file: str
        self.possible_published_results: list[str]
        self.depfile: str
        self.depfile_target: str
        self.log_level: int
        self.pkg_type: str


def process_possible_published_results(file_paths: list[str]) -> list[str]:
    results = []
    for base_path in file_paths:
        full_path = os.path.join(base_path, "published_results")
        if os.path.exists(full_path):
            with open(full_path, "r") as file:
                content = file.read()
                results.append(content)
    return results


if __name__ == "__main__":
    parser = argparse.ArgumentParser()

    parser.add_argument(
        "--tman-path",
        type=str,
        required=True,
        help="Path to tman command itself",
    )
    parser.add_argument(
        "--pkg-type",
        type=str,
        required=True,
        help="The type of package",
        choices=["app", "extension"],
    )
    parser.add_argument(
        "--pkg-src-root-dir",
        type=str,
        required=True,
        help="Path to app source root directory",
    )
    parser.add_argument(
        "--tg-timestamp-proxy-file",
        type=str,
        required=True,
        help="Path to the stamp file",
    )
    parser.add_argument(
        "--src-pkg",
        type=str,
        required=True,
        help="The name of TEN package",
    )
    parser.add_argument(
        "--build-type",
        type=str,
        required=False,
        help="'debug', 'release'",
    )
    parser.add_argument(
        "--config-file",
        type=str,
        required=False,
        help="Path of tman config file",
    )
    parser.add_argument(
        "--possible-published-results", type=str, action="append", default=[]
    )
    parser.add_argument("--depfile", type=str)
    parser.add_argument("--depfile-target", type=str)
    parser.add_argument(
        "--log-level", type=int, required=True, help="specify log level"
    )

    arg_info = ArgumentInfo()
    args = parser.parse_args(namespace=arg_info)

    if args.log_level > 0:
        print(f"> Install {args.pkg_type}: {args.pkg_src_root_dir}")

    published_results = process_possible_published_results(
        args.possible_published_results
    )
    if published_results:
        write_depfile.write_depfile(
            args.depfile_target,
            published_results,
            args.depfile,
        )

    origin_wd = os.getcwd()
    returncode = 0

    generated_app_name = os.path.basename(args.pkg_src_root_dir)

    try:
        # Install app.
        base_dir = os.path.dirname(args.pkg_src_root_dir)
        os.makedirs(base_dir, exist_ok=True)
        os.chdir(base_dir)

        cmd = [
            args.tman_path,
        ]

        if args.config_file is not None:
            list.append(cmd, "--config-file=" + args.config_file)

        cmd += [
            "install",
            args.pkg_type,
            args.src_pkg,
            "--template-mode",
            "--template-data",
            f"package_name={generated_app_name}",
        ]

        if args.build_type is not None:
            list.append(cmd, "--build-type=" + args.build_type)

        returncode, _ = cmd_exec.run_cmd(cmd, args.log_level)
        if returncode:
            raise Exception("Failed to install app.")
        else:
            timestamp_proxy.touch_timestamp_proxy_file(
                args.tg_timestamp_proxy_file
            )

    except Exception as exc:
        returncode = 1
        timestamp_proxy.remove_timestamp_proxy_file(
            args.tg_timestamp_proxy_file
        )
        print(exc)

    finally:
        os.chdir(origin_wd)
        sys.exit(returncode)
