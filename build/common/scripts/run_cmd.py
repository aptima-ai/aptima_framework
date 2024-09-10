#
# This file is part of the TEN Framework project.
# See https://github.com/TEN-framework/ten_framework/LICENSE for license
# information.
#
import sys
from build.scripts import cmd_exec


if __name__ == "__main__":
    cmd_exec.run_cmd(sys.argv[1:])
