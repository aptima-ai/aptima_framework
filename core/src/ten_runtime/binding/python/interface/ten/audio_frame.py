#
# This file is part of the TEN Framework project.
# See https://github.com/TEN-framework/ten_framework/LICENSE for license
# information.
#
from libten_runtime_python import _AudioFrame
from enum import IntEnum


# AudioFrameDataFmt values. These definitions need to be the same as the
# TEN_AUDIO_FRAME_DATA_FMT enum in C.
#
# Note: To achieve the best compatibility, any new enum item, should be added
# to the end to avoid changing the value of previous enum items.
class AudioFrameDataFmt(IntEnum):
    INTERLEAVE = 1
    NON_INTERLEAVE = 2


class AudioFrame(_AudioFrame):
    def __init__(self):
        raise NotImplementedError("Use AudioFrame.create instead.")

    @classmethod
    def create(cls, msg_name: str):
        instance = cls.__new__(cls)
        instance.set_name(msg_name)
        return instance

    def alloc_buf(self, size: int) -> None:
        return _AudioFrame.alloc_buf(self, size)

    def lock_buf(self) -> memoryview:
        return _AudioFrame.lock_buf(self)

    def unlock_buf(self, buf: memoryview) -> None:
        return _AudioFrame.unlock_buf(self, buf)

    def get_buf(self) -> bytearray:
        return _AudioFrame.get_buf(self)

    def get_timestamp(self) -> int:
        return _AudioFrame.get_timestamp(self)

    def set_timestamp(self, timestamp: int) -> None:
        return _AudioFrame.set_timestamp(self, timestamp)

    def get_sample_rate(self) -> int:
        return _AudioFrame.get_sample_rate(self)

    def set_sample_rate(self, sample_rate: int) -> None:
        return _AudioFrame.set_sample_rate(self, sample_rate)

    def get_samples_per_channel(self) -> int:
        return _AudioFrame.get_samples_per_channel(self)

    def set_samples_per_channel(self, samples_per_channel: int) -> None:
        return _AudioFrame.set_samples_per_channel(self, samples_per_channel)

    def get_bytes_per_sample(self) -> int:
        return _AudioFrame.get_bytes_per_sample(self)

    def set_bytes_per_sample(self, bytes_per_sample: int) -> None:
        return _AudioFrame.set_bytes_per_sample(self, bytes_per_sample)

    def get_number_of_channels(self) -> int:
        return _AudioFrame.get_number_of_channels(self)

    def set_number_of_channels(self, number_of_channels: int) -> None:
        return _AudioFrame.set_number_of_channels(self, number_of_channels)

    def get_data_fmt(self) -> AudioFrameDataFmt:
        return AudioFrameDataFmt(_AudioFrame.get_data_fmt(self))

    def set_data_fmt(self, data_fmt: AudioFrameDataFmt) -> None:
        return _AudioFrame.set_data_fmt(self, data_fmt.value)

    def get_line_size(self) -> int:
        return _AudioFrame.get_line_size(self)

    def set_line_size(self, line_size: int) -> None:
        return _AudioFrame.set_line_size(self, line_size)

    def get_is_eof(self) -> bool:
        return _AudioFrame.get_is_eof(self)

    def set_is_eof(self, is_eof: bool) -> None:
        return _AudioFrame.set_is_eof(self, is_eof)
