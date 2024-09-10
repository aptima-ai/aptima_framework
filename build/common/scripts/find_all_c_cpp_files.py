#
# This file is part of the TEN Framework project.
# See https://github.com/TEN-framework/ten_framework/LICENSE for license
# information.
#
import sys
import os


def main(argv):
    if len(argv) != 1:
        raise Exception("Invalid parameter")

    path = os.path.abspath(argv[0])

    pattern = [
        ".h",
        ".hpp",
        ".c",
        ".cpp",
        ".cc",
    ]

    for dirpath, _, filenames in os.walk(path):
        for file in filenames:
            for p in pattern:
                if str(file).endswith(p):
                    print(os.path.join(path, dirpath, file))


if __name__ == "__main__":
    main(sys.argv[1:])
