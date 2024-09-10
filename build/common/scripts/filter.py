#
# This file is part of the TEN Framework project.
# See https://github.com/TEN-framework/ten_framework/LICENSE for license
# information.
#
import sys
import fnmatch


def main(argv):
    if len(argv) < 1:
        raise Exception("Invalid parameter")
    re_str = argv[0]
    argv = argv[1:]
    for v in fnmatch.filter(argv, re_str):
        sys.stdout.write(v.replace("\\", "/") + "\n")
    sys.exit(0)


if __name__ == "__main__":
    main(sys.argv[1:])
