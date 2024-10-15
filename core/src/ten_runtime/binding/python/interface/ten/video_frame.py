#
# Copyright © 2024 Agora
# This file is part of TEN Framework, an open source project.
# Licensed under the Apache License, Version 2.0, with certain conditions.
# Refer to the "LICENSE" file in the root directory for more information.
#
from libten_runtime_python import _VideoFrame
from enum import IntEnum


# PixelFmt values. These definitions need to be the same as the PixelFmt
# enum in C.
#
# Note: To achieve the best compatibility, any new enum item, should be added
# to the end to avoid changing the value of previous enum items.
class PixelFmt(IntEnum):
    RGB24 = 1
    RGBA = 2
    BGR24 = 3
    BGRA = 4
    I422 = 5
    I420 = 6
    NV21 = 7
    NV12 = 8


class VideoFrame(_VideoFrame):
    def __init__(self):
        raise NotImplementedError("Use VideoFrame.create instead.")

    @classmethod
    def create(cls, msg_name: str):
        instance = cls.__new__(cls)
        instance.set_name(msg_name)
        return instance

    def alloc_buf(self, size: int) -> None:
        return _VideoFrame.alloc_buf(self, size)

    def lock_buf(self) -> memoryview:
        return _VideoFrame.lock_buf(self)

    def unlock_buf(self, buf: memoryview) -> None:
        return _VideoFrame.unlock_buf(self, buf)

    def get_buf(self) -> bytearray:
        return _VideoFrame.get_buf(self)

    def get_width(self) -> int:
        return _VideoFrame.get_width(self)

    def set_width(self, width: int) -> None:
        return _VideoFrame.set_width(self, width)

    def get_height(self) -> int:
        return _VideoFrame.get_height(self)

    def set_height(self, height: int) -> None:
        return _VideoFrame.set_height(self, height)

    def get_timestamp(self) -> int:
        return _VideoFrame.get_timestamp(self)

    def set_timestamp(self, timestamp: int) -> None:
        return _VideoFrame.set_timestamp(self, timestamp)

    def get_pixel_fmt(self) -> PixelFmt:
        return PixelFmt(_VideoFrame.get_pixel_fmt(self))

    def set_pixel_fmt(self, pixel_fmt: PixelFmt) -> None:
        return _VideoFrame.set_pixel_fmt(self, pixel_fmt.value)

    def is_eof(self) -> bool:
        return _VideoFrame.is_eof(self)

    def set_eof(self, eof: bool) -> None:
        return _VideoFrame.set_eof(self, eof)
