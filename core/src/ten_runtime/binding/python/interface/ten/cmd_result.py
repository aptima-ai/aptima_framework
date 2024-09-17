#
# Copyright © 2024 Agora
# This file is part of TEN Framework, an open source project.
# Licensed under the Apache License, Version 2.0, with certain conditions.
# Refer to the "LICENSE" file in the root directory for more information.
#
from libten_runtime_python import _CmdResult
from enum import IntEnum


# StatusCode values. These definitions need to be the same as the
# TEN_STATUS_CODE enum in C.
#
# Note: To achieve the best compatibility, any new enum item, should be added
# to the end to avoid changing the value of previous enum items.
class StatusCode(IntEnum):
    OK = 0
    ERROR = 1


class CmdResult(_CmdResult):
    def __init__(self):
        raise NotImplementedError("Use CmdResult.create instead.")

    @classmethod
    def create(cls, status_code: StatusCode):
        instance = cls.__new__(cls)
        instance.set_status_code(status_code)
        return instance

    def get_status_code(self) -> StatusCode:
        return StatusCode(_CmdResult.get_status_code(self))

    def set_is_final(self, is_final: bool):
        if is_final:
            return _CmdResult.set_is_final(self, 1)
        else:
            return _CmdResult.set_is_final(self, 0)

    def get_is_final(self) -> bool:
        return _CmdResult.get_is_final(self)
