#
# This file is part of the TEN Framework project.
# See https://github.com/TEN-framework/ten_framework/LICENSE for license
# information.
#
from enum import IntEnum


class _TenEnvAttachTo(IntEnum):
    EXTENSION = 1
    EXTENSION_GROUP = 2
    APP = 3
    ADDON = 4
