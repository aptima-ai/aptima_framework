#
# Copyright © 2024 Agora
# This file is part of TEN Framework, an open source project.
# Licensed under the Apache License, Version 2.0, with certain conditions.
# Refer to the "LICENSE" file in the root directory for more information.
#
import os
import argparse
import re
import sys
from build.scripts import cmd_exec, touch
from common.scripts import delete_files
import subprocess


class ArgumentInfo(argparse.Namespace):
    def __init__(self):
        self.tman_path: str
        self.published_results: str
        self.base_dir: str
        self.config_file: str
        self.log_level: int
        self.enable_publish: bool
        self.os: str
        self.cpu: str


def extract_publish_path(text: str) -> str | None:
    pattern = r'Publish to "(.+?)"'
    match = re.search(pattern, text)
    return match.group(1) if match is not None else None


def write_published_results_to_file(
    published_results: str, file_path: str
) -> None:
    with open(file_path, "w") as file:
        file.write(published_results)


def update_manifest(
    base_dir: str, os_str: str, cpu_str: str, log_level: int
) -> None:
    manifest_path = os.path.join(base_dir, "manifest.json")

    os_arch_pair = f"{os_str}:{cpu_str}"

    script_dir = os.path.dirname(os.path.abspath(__file__))
    update_script_path = os.path.abspath(
        os.path.join(
            script_dir,
            "../../../tools/supports/update_supports_in_manifest_json.py",
        )
    )

    try:
        subprocess.run(
            [
                "python",
                update_script_path,
                "--input-file",
                manifest_path,
                "--output-file",
                manifest_path,
                "--os-arch-pairs",
                os_arch_pair,
                "--log-level",
                str(log_level),
            ],
            check=True,
        )
        if log_level > 0:
            print(
                f"Manifest updated successfully with os/cpu: {os_str}/{cpu_str}"
            )
    except subprocess.CalledProcessError as e:
        print(f"Failed to update manifest.json: {e}")
        sys.exit(1)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()

    parser.add_argument(
        "--tman-path",
        type=str,
        required=True,
        help=(
            "Path to the tman executable. The tman is not compiled on ios or"
            " android, so it's optional."
        ),
    )
    parser.add_argument("--published-results", type=str, required=True)
    parser.add_argument("--base-dir", type=str, required=False)
    parser.add_argument(
        "--config-file",
        default="",
        type=str,
        help="Path of tman config file",
    )
    parser.add_argument(
        "--log-level", type=int, required=False, help="specify log level"
    )
    parser.add_argument(
        "--enable-publish",
        action=argparse.BooleanOptionalAction,
        default=True,
    )
    parser.add_argument("--os", type=str, required=True)
    parser.add_argument("--cpu", type=str, required=True)

    arg_info = ArgumentInfo()
    args = parser.parse_args(namespace=arg_info)

    if args.enable_publish is False:
        sys.exit(0)

    update_manifest(args.base_dir, args.os, args.cpu, args.log_level)

    # Use 'tman publish' to perform the uploading.
    origin_wd = os.getcwd()

    if args.log_level > 0:
        print(f"> Publish: {args.base_dir}")

    returncode = 0

    try:
        os.chdir(os.path.abspath(args.base_dir))

        cmd = [
            args.tman_path,
            "--config-file=" + args.config_file,
            "--mi",
            "publish",
        ]
        returncode, output_text = cmd_exec.run_cmd(cmd)
        if returncode:
            print(output_text)
            raise Exception(f"Failed to publish package: {returncode}")

        published_results = extract_publish_path(output_text)
        if published_results is None:
            print(output_text)
            raise Exception(f"Failed to publish package: {returncode}")

        if args.log_level > 0:
            print(f"Published to {published_results}")

        write_published_results_to_file(
            published_results, args.published_results
        )

        touch.touch(os.path.join(f"{args.published_results}"))

    except Exception as e:
        delete_files.delete_files([os.path.join(f"{args.published_results}")])
        print(e)

    finally:
        os.chdir(origin_wd)
        sys.exit(-1 if returncode != 0 else 0)
