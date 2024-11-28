#
# Copyright © 2024 Agora
# This file is part of TEN Framework, an open source project.
# Licensed under the Apache License, Version 2.0, with certain conditions.
# Refer to the "LICENSE" file in the root directory for more information.
#
from libten_runtime_python import _Cmd


class Cmd(_Cmd):
    def __new__(cls, name: str):
        return super().__new__(cls, name)

    def __init__(self):
        raise NotImplementedError("Use Cmd.create instead.")

    @classmethod
    def create(cls, name: str):
        return cls.__new__(cls, name)
