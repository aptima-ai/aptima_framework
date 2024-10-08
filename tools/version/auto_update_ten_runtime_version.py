#
# Copyright © 2024 Agora
# This file is part of TEN Framework, an open source project.
# Licensed under the Apache License, Version 2.0, with certain conditions.
# Refer to the "LICENSE" file in the root directory for more information.
#
import datetime
import json
import os
from update_version_from_git import (
    get_latest_git_tag,
    update_c_preserved_metadata_file,
    update_version_in_manifest_json_file,
    update_dependency_manifest_json_file,
    update_tman_version_source_file,
    PkgInfo,
)

MANIFEST_JSON_FILE = "manifest.json"
MANIFEST_JSON_TENT_FILE = "manifest.json.tent"


def __get_pkg_info_from_manifest_file(manifest_file: str) -> PkgInfo:
    with open(manifest_file, "r") as f:
        manifest = json.load(f)
        return PkgInfo(manifest["type"], manifest["name"])


def __collect_manifests(directory) -> list[str]:
    manifests = []

    for root, dirs, files in os.walk(directory, followlinks=True):
        for dir in dirs:
            if os.path.exists(os.path.join(directory, dir, MANIFEST_JSON_FILE)):
                manifests.append(
                    os.path.join(
                        directory,
                        dir,
                        MANIFEST_JSON_FILE,
                    )
                )

        break

    return manifests


def __collect_manifest_tents(directory) -> list[str]:
    manifest_templates = []

    for root, dirs, files in os.walk(directory, followlinks=True):
        for dir in dirs:
            if os.path.exists(
                os.path.join(directory, dir, MANIFEST_JSON_TENT_FILE)
            ):
                manifest_templates.append(
                    os.path.join(
                        directory,
                        dir,
                        MANIFEST_JSON_TENT_FILE,
                    )
                )

        break

    return manifest_templates


def update_ten_runtime_binary_version(log_level, year, year_month, git_version):
    # Update the version in the C preserved metadata files.
    c_preserved_metadata_file_src_file = os.path.join(
        repo_base_dir,
        "core",
        "src",
        "ten_runtime",
        "build_template",
        "preserved_metadata.c",
    )

    c_preserved_metadata_file_template_file = os.path.join(
        repo_base_dir,
        "core",
        "src",
        "ten_runtime",
        "build_template",
        "preserved_metadata.c.jinja2",
    )

    update_c_preserved_metadata_file(
        log_level,
        year,
        year_month,
        git_version,
        c_preserved_metadata_file_src_file,
        c_preserved_metadata_file_template_file,
    )


def update_tman_version(
    log_level: int,
    year: str,
    year_month: str,
    repo_base_dir: str,
    git_version: str,
):
    tman_version_src_file_path = os.path.join(
        repo_base_dir,
        "core",
        "src",
        "ten_manager",
        "src",
        "version.rs",
    )

    tman_version_template_file_path = os.path.join(
        repo_base_dir, "core", "src", "ten_manager", "src", "version.rs.jinja2"
    )

    update_tman_version_source_file(
        log_level,
        year,
        year_month,
        git_version,
        tman_version_src_file_path,
        tman_version_template_file_path,
    )


def collect_ten_system_packages_and_update_version(
    log_level, repo_base_dir, git_version
) -> list[PkgInfo]:
    # Collect manifest files for ten_runtime and all corresponding system
    # packages(python & go bindings).
    manifest_files = [
        # ten_runtime
        os.path.join(
            repo_base_dir,
            "core",
            "src",
            "ten_runtime",
            MANIFEST_JSON_FILE,
        ),
        # ten_runtime_go
        os.path.join(
            repo_base_dir,
            "core",
            "src",
            "ten_runtime",
            "binding",
            "go",
            MANIFEST_JSON_FILE,
        ),
        # ten_runtime_python
        os.path.join(
            repo_base_dir,
            "core",
            "src",
            "ten_runtime",
            "binding",
            "python",
            MANIFEST_JSON_FILE,
        ),
    ]

    pkgInfos = []

    for manifest_file in manifest_files:
        update_version_in_manifest_json_file(
            log_level, git_version, manifest_file
        )
        pkgInfos.append(__get_pkg_info_from_manifest_file(manifest_file))

    return pkgInfos


def collect_core_packages_and_update_version(
    log_level, repo_base_dir, git_version
) -> list[PkgInfo]:
    # Collect manifest files for all core packages.
    core_apps_dir_path = os.path.join(
        repo_base_dir,
        "packages",
        "core_apps",
    )
    core_extensions_dir_path = os.path.join(
        repo_base_dir,
        "packages",
        "core_extensions",
    )
    core_protocols_dir_path = os.path.join(
        repo_base_dir,
        "packages",
        "core_protocols",
    )

    manifest_files = (
        __collect_manifests(core_apps_dir_path)
        + __collect_manifests(core_extensions_dir_path)
        + __collect_manifests(core_protocols_dir_path)
    )

    manifest_template_files = (
        __collect_manifest_tents(core_apps_dir_path)
        + __collect_manifest_tents(core_extensions_dir_path)
        + __collect_manifest_tents(core_protocols_dir_path)
    )

    pkgInfos = []

    for manifest_file in manifest_files:
        update_version_in_manifest_json_file(
            log_level, git_version, manifest_file
        )
        pkgInfos.append(__get_pkg_info_from_manifest_file(manifest_file))

    for manifest_template_file in manifest_template_files:
        update_version_in_manifest_json_file(
            log_level, git_version, manifest_template_file
        )

    return pkgInfos


def update_package_dependencies(
    log_level, repo_base_dir, git_version, dependencies
):
    # Collect manifest files for system packages.
    system_package_dir_path = os.path.join(
        repo_base_dir, "core", "src", "ten_runtime", "binding"
    )

    # Collect manifest files for all packages.
    packages_dir_path = os.path.join(
        repo_base_dir,
        "packages",
    )

    # Collect manifest files for testing packages.
    test_dir_path = os.path.join(repo_base_dir, "tests")

    manifests = __collect_manifests(system_package_dir_path)

    for root, dirs, files in os.walk(packages_dir_path, followlinks=True):
        for dir in dirs:
            manifests += __collect_manifests(
                os.path.join(packages_dir_path, dir)
            )
            manifests += __collect_manifest_tents(
                os.path.join(packages_dir_path, dir)
            )

        break

    for root, dirs, files in os.walk(test_dir_path, followlinks=True):
        for file in files:
            if file == MANIFEST_JSON_FILE:
                manifests.append(os.path.join(root, file))

    for manifest in manifests:
        update_dependency_manifest_json_file(
            log_level,
            git_version,
            manifest,
            dependencies,
        )


if __name__ == "__main__":
    now = datetime.datetime.now()
    year = now.strftime("%Y")
    year_month = now.strftime("%Y-%m")

    # Get the repo base directory.
    repo_base_dir = os.path.abspath(
        os.path.join(os.path.dirname(__file__), "..", "..")
    )

    # Change to the correct directory to get the correct git tag.
    os.chdir(repo_base_dir)

    # Get the latest Git tag, and optionally strip leading 'v' if present.
    git_version = get_latest_git_tag()
    git_version = git_version.lstrip("v")

    log_level = 1

    update_ten_runtime_binary_version(log_level, year, year_month, git_version)

    update_tman_version(log_level, year, year_month, repo_base_dir, git_version)

    system_pkgs = collect_ten_system_packages_and_update_version(
        log_level, repo_base_dir, git_version
    )

    core_pkgs = collect_core_packages_and_update_version(
        log_level, repo_base_dir, git_version
    )

    update_package_dependencies(
        log_level, repo_base_dir, git_version, system_pkgs + core_pkgs
    )
