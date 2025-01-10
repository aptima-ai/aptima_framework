#
# Copyright © 2025 Agora
# This file is part of APTIMA Framework, an open source project.
# Licensed under the Apache License, Version 2.0, with certain conditions.
# Refer to the "LICENSE" file in the root directory for more information.
#
from typing import Type, TypeVar

from libaxis_runtime_python import _Data

T = TypeVar("T", bound="Data")


class Data(_Data):
    def __init__(self):
        raise NotImplementedError("Use Data.create instead.")

    @classmethod
    def create(cls: Type[T], name: str) -> T:
        return cls.__new__(cls, name)

    alloc_buf = _Data.alloc_buf
    lock_buf = _Data.lock_buf
    unlock_buf = _Data.unlock_buf
    get_buf = _Data.get_buf
