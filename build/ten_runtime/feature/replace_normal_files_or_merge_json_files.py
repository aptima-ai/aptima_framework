#
# This file is part of the TEN Framework project.
# See https://github.com/TEN-framework/ten_framework/LICENSE for license
# information.
#
import argparse
import json
import os
import sys
from build.scripts import fs_utils


class ArgumentInfo(argparse.Namespace):
    def __init__(self):
        self.replaced_files: list[str]


def merge_json_file_at_root_level(src: str, dest: str) -> None:
    if not os.path.exists(src):
        raise Exception(f"{src} does not exist.")
    if not os.path.exists(dest):
        raise Exception(f"{dest} does not exist.")

    with open(src, "r") as src_file, open(dest, "r+") as dest_file:
        try:
            src_file_content = json.load(src_file)
        except Exception as e:
            print(f"{src_file} contains invalid JSON: {e}")
            raise e

        try:
            dest_file_content = json.load(dest_file)
        except Exception as e:
            print(f"{dest_file} contains invalid JSON: {e}")
            raise e

        for key, value in src_file_content.items():
            dest_file_content[key] = value

        dest_file.seek(0)
        json.dump(dest_file_content, dest_file, indent=2)
        dest_file.truncate()


def split(input: str, split_key: str) -> tuple[str, str]:
    split_key_index = str(input).find(split_key)
    if split_key_index == -1:
        # split_key is not found, this means the source and the destination
        # are equal.
        return (input, input)
    else:
        return (
            str(input)[:split_key_index],
            str(input)[split_key_index + len(split_key) :],
        )


def replace_normal_files_or_merge_json_files(replaced_files: list[str]) -> None:
    for res in replaced_files:
        res_src, res_dest = split(res, "=>")

        if not os.path.exists(res_src):
            raise Exception(f"{res_src} does not exist.")

        if (
            res_src.endswith(".json")
            and res_dest.endswith(".json")
            and os.path.exists(res_dest)
        ):
            merge_json_file_at_root_level(res_src, res_dest)
        else:
            fs_utils.copy(res_src, res_dest)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()

    parser.add_argument(
        "--replaced-files",
        type=str,
        action="append",
        default=[],
        help="Resources to be replaced",
    )

    arg_info = ArgumentInfo()
    args = parser.parse_args(namespace=arg_info)

    origin_wd = os.getcwd()
    returncode = 0

    try:
        replace_normal_files_or_merge_json_files(args.replaced_files)
    except Exception as exc:
        returncode = 1
        print(exc)
    finally:
        os.chdir(origin_wd)
        sys.exit(returncode)
