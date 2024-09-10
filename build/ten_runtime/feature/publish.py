#
# This file is part of the TEN Framework project.
# See https://github.com/TEN-framework/ten_framework/LICENSE for license
# information.
#
import os
import argparse
import re
import sys
from build.scripts import cmd_exec, touch
from common.scripts import delete_files


class ArgumentInfo(argparse.Namespace):
    def __init__(self):
        self.tman_path: str
        self.published_results: str
        self.base_dir: str
        self.config_file: str
        self.log_level: int
        self.enable_publish: bool


def extract_publish_path(text: str) -> str | None:
    pattern = r'Publish to "(.+?)"'
    match = re.search(pattern, text)
    return match.group(1) if match is not None else None


def write_published_results_to_file(
    published_results: str, file_path: str
) -> None:
    with open(file_path, "w") as file:
        file.write(published_results)


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

    arg_info = ArgumentInfo()
    args = parser.parse_args(namespace=arg_info)

    if args.enable_publish is False:
        sys.exit(0)

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
