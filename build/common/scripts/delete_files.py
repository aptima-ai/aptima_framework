#
# This file is part of the TEN Framework project.
# See https://github.com/TEN-framework/ten_framework/LICENSE for license
# information.
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
