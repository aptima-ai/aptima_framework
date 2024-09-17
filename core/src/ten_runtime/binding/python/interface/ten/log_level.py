#
# This file is part of the TEN Framework project.
# See https://github.com/TEN-framework/ten_framework/LICENSE for license
# information.
#
from enum import IntEnum


class LogLevel(IntEnum):
    INVALID = 0
    VERBOSE = 1
    DEBUG = 2
    INFO = 3
    WARN = 4
    ERROR = 5
    FATAL = 6
