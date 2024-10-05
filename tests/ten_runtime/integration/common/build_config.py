#
# Copyright © 2024 Agora
# This file is part of TEN Framework, an open source project.
# Licensed under the Apache License, Version 2.0, with certain conditions.
# Refer to the "LICENSE" file in the root directory for more information.
#
class BuildConfig:
    def __init__(self, target_os, target_cpu, target_build):
        self.target_os = target_os
        self.target_cpu = target_cpu
        self.target_build = target_build


def parse_build_config(file_path: str):
    target_os = None
    target_cpu = None
    is_debug = None

    with open(file_path, "r") as file:
        for line in file:
            line = line.strip()
            if line.startswith("target_os"):
                target_os = line.split("=")[1].strip().strip('"')
            elif line.startswith("target_cpu"):
                target_cpu = line.split("=")[1].strip().strip('"')
            elif line.startswith("is_debug"):
                is_debug = line.split("=")[1].strip().lower() == "true"

    target_build = "debug" if is_debug else "release"

    return BuildConfig(target_os, target_cpu, target_build)
