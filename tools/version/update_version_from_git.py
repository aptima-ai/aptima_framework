#
# This file is part of the TEN Framework project.
# See https://github.com/TEN-framework/ten_framework/LICENSE for license
# information.
#
import subprocess
import json
import argparse
import os
import datetime
from jinja2 import Template
from build.scripts import touch


class ArgumentInfo(argparse.Namespace):
    def __init__(self):
        super().__init__()
        self.repo_base_dir: str
        self.log_level: int
        self.pkg_type: str | None = None
        self.pkg_name: str | None = None
        self.c_preserved_metadata_path: list[tuple[str, str]] = []
        self.c_preserved_metadata_not_in_output_path: list[tuple[str, str]] = []
        self.version_update_manifest_path: list[tuple[str, str]] = []
        self.dependency_version_update_manifest_path: list[tuple[str, str]] = []


def get_latest_git_tag() -> str:
    result = subprocess.run(
        ["git", "describe", "--tags", "--abbrev=0"],
        capture_output=True,
        text=True,
    )
    if result.returncode != 0:
        raise Exception("Failed to execute git command")
    return result.stdout.strip()


def update_c_preserved_metadata_file(
    args: ArgumentInfo,
    year: str,
    year_month: str,
    version: str,
    src_path: str,
    dest_path: str,
) -> None:
    if args.log_level > 0:
        print(f"Checking {dest_path} for updates...")

    update_needed = False

    if os.path.exists(dest_path):
        with open(dest_path, "r") as file:
            content = file.read()

        if f"version={version}" not in content:
            update_needed = True
            if args.log_level > 0:
                print(
                    f"Version mismatch found. Updating version in {dest_path}."
                )
    else:
        update_needed = True

    if update_needed:
        with open(src_path, "r") as file:
            template_content = file.read()

        template = Template(template_content)

        rendered_content = template.render(
            VERSION=version, YEAR=year, YEAR_MONTH=year_month
        )

        with open(dest_path, "w") as file:
            file.write(rendered_content)
    else:
        if args.log_level > 0:
            print(f"No update needed for {dest_path}; versions match.")


# {
#   "type": "<type-in-main-manifest>",
#   "name": "<name-in-main-manifest>",
#   "version": "...", <- update here
#   ...
# }
def update_version_in_manifest_json_file(
    args: ArgumentInfo,
    version: str,
    src_path: str,
    dest_path: str,
) -> None:
    if args.log_level > 0:
        print(f"Checking {src_path} for updates...")

    if not os.path.exists(dest_path):
        touch.touch(dest_path)

    with open(src_path, "r") as file:
        data = json.load(file)

    if data.get("version") != version:
        if args.log_level > 0:
            print(f"Updating version in {src_path}.")

        # Update the version in the JSON data.
        data["version"] = version

        with open(src_path, "w") as file:
            json.dump(data, file, indent=2)

            # Notify that the content of the src_path has changed.
            touch.touch(dest_path)
    else:
        if args.log_level > 0:
            print(f"No update needed for {src_path}; versions match.")


# {
#   ...
#   "dependencies": [
#     {
#       "type": "<type-in-main-manifest>",
#       "name": "<name-in-main-manifest>",
#       "version": "...", <- update here
#     }
#   }
#   ...
# }
def update_dependency_manifest_json_file(
    args: ArgumentInfo,
    version: str,
    src_path: str,
    dest_path: str,
) -> None:
    if args.log_level > 0:
        print(f"Checking {src_path} for updates...")

    if not os.path.exists(dest_path):
        touch.touch(dest_path)

    with open(src_path, "r") as file:
        data = json.load(file)

    updated = False
    for dependency in data.get("dependencies", []):
        if (
            dependency.get("type") == args.pkg_type
            and dependency.get("name") == args.pkg_name
        ):
            dependency["version"] = version
            updated = True

    if updated:
        if args.log_level > 0:
            print(f"Updating version in {src_path}.")

        with open(src_path, "w") as file:
            json.dump(data, file, indent=2)

            # Notify that the content of the src_path has changed.
            touch.touch(dest_path)
    else:
        if args.log_level > 0:
            print(f"No update needed for {src_path}; versions match.")


def main(args: ArgumentInfo):
    now = datetime.datetime.now()
    year = now.strftime("%Y")
    year_month = now.strftime("%Y-%m")

    # Change to the correct directory to get the correct git tag.
    os.chdir(args.repo_base_dir)

    # Get the latest Git tag, and optionally strip leading 'v' if present.
    git_version = get_latest_git_tag()
    git_version = git_version.lstrip("v")

    if args.c_preserved_metadata_path:
        for src, dest in args.c_preserved_metadata_path:
            update_c_preserved_metadata_file(
                args, year, year_month, git_version, src, dest
            )

    if args.c_preserved_metadata_not_in_output_path:
        for src, dest in args.c_preserved_metadata_not_in_output_path:
            update_c_preserved_metadata_file(
                args, year, year_month, git_version, src, dest
            )

    if args.version_update_manifest_path:
        for src, dest in args.version_update_manifest_path:
            update_version_in_manifest_json_file(args, git_version, src, dest)

    if args.dependency_version_update_manifest_path:
        for src, dest in args.dependency_version_update_manifest_path:
            update_dependency_manifest_json_file(args, git_version, src, dest)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()

    parser.add_argument("--repo-base-dir", type=str, required=True)
    parser.add_argument("--log-level", type=int, required=True)

    parser.add_argument("--pkg-type", type=str)
    parser.add_argument("--pkg-name", type=str)

    parser.add_argument(
        "--c-preserved-metadata-path",
        action="append",
        nargs=2,
        metavar=("src", "dest"),
    )
    parser.add_argument(
        "--c-preserved-metadata-not-in-output-path",
        action="append",
        nargs=2,
        metavar=("src", "dest"),
    )
    parser.add_argument(
        "--version-update-manifest-path",
        action="append",
        nargs=2,
        metavar=("src", "dest"),
    )
    parser.add_argument(
        "--dependency-version-update-manifest-path",
        action="append",
        nargs=2,
        metavar=("src", "dest"),
    )

    arg_info = ArgumentInfo()
    args = parser.parse_args(namespace=arg_info)

    if args.dependency_version_update_manifest_path and (
        not args.pkg_type or not args.pkg_name
    ):
        error_message = (
            "--pkg-type and --pkg-name are required when "
            "--dependency-version-update-manifest-path is specified"
        )
        parser.error(error_message)

    main(args)
