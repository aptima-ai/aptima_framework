#
# Copyright © 2024 Agora
# This file is part of TEN Framework, an open source project.
# Licensed under the Apache License, Version 2.0, with certain conditions.
# Refer to the "LICENSE" file in the root directory for more information.
#
import sys
import os
import shutil


def delete_files(files: list[str]):
    for file in files:
        if not os.path.exists(file):
            continue

        if os.path.isfile(file):
            os.remove(file)
        else:
            shutil.rmtree(file)


if __name__ == "__main__":
    delete_files(sys.argv[1:])
