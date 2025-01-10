#
# Copyright © 2025 Agora
# This file is part of APTIMA Framework, an open source project.
# Licensed under the Apache License, Version 2.0, with certain conditions.
# Refer to the "LICENSE" file in the root directory for more information.
#
from typing import Type, TypeVar

from libaxis_runtime_python import _Cmd

T = TypeVar("T", bound="Cmd")


class Cmd(_Cmd):
    def __init__(self):
        raise NotImplementedError("Use Cmd.create instead.")

    @classmethod
    def create(cls: Type[T], name: str) -> T:
        return cls.__new__(cls, name)
