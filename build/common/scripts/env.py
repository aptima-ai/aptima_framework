#
# This file is part of the TEN Framework project.
# See https://github.com/TEN-framework/ten_framework/LICENSE for license
# information.
#
import os


def dump_env_vars():
    for k, v in os.environ.items():
        print(f"{k}={v}")
