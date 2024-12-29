#
# Copyright © 2025 Agora
# This file is part of TEN Framework, an open source project.
# Licensed under the Apache License, Version 2.0, with certain conditions.
# Refer to the "LICENSE" file in the root directory for more information.
#
import sys
from build.scripts import fs_utils, log


def main():
    if len(sys.argv) != 2:
        log.error("Usage: mkdir.py <dir>")
        sys.exit(1)

    dir_path = sys.argv[1]

    try:
        fs_utils.mkdir_p(dir_path)
        log.info(f"Create successfully: {dir_path}")
    except Exception as e:
        log.error(f"Failed to create directory: {dir_path}\n: {e}")
        sys.exit(1)


if __name__ == "__main__":
    main()
