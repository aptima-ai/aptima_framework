#
# Copyright © 2025 Agora
# This file is part of TEN Framework, an open source project.
# Licensed under the Apache License, Version 2.0, with certain conditions.
# Refer to the "LICENSE" file in the root directory for more information.
#
from typing import Callable, Optional

from libten_runtime_python import _Extension, _TenEnv

from .audio_frame import AudioFrame
from .cmd import Cmd
from .cmd_result import CmdResult
from .data import Data
from .error import TenError
from .ten_env_base import TenEnvBase
from .video_frame import VideoFrame


class TenEnv: ...  # type: ignore


ResultHandler = Optional[
    Callable[[TenEnv, Optional[CmdResult], Optional[TenError]], None]
]

ErrorHandler = Optional[Callable[[TenEnv, Optional[TenError]], None]]


class TenEnv(TenEnvBase):

    def __init__(self, internal_obj: _TenEnv) -> None:
        super().__init__(internal_obj)

    def __del__(self) -> None:
        pass

    def _set_release_handler(self, handler: Callable[[], None]) -> None:
        self._release_handler = handler

    def _on_release(self) -> None:
        if hasattr(self, "_release_handler"):
            self._release_handler()

    def on_configure_done(self) -> None:
        return self._internal.on_configure_done()

    def on_init_done(self) -> None:
        return self._internal.on_init_done()

    def on_start_done(self) -> None:
        return self._internal.on_start_done()

    def on_stop_done(self) -> None:
        return self._internal.on_stop_done()

    def on_deinit_done(self) -> None:
        return self._internal.on_deinit_done()

    def on_create_instance_done(self, instance: _Extension, context) -> None:
        return self._internal.on_create_instance_done(instance, context)

    def get_property_to_json(self, path: str) -> str:
        return self._internal.get_property_to_json(path)

    def set_property_from_json(self, path: str, json_str: str) -> None:
        return self._internal.set_property_from_json(path, json_str)

    def send_cmd(self, cmd: Cmd, result_handler: ResultHandler) -> None:
        self._internal.send_cmd(cmd, result_handler, False)

    def send_cmd_ex(self, cmd: Cmd, result_handler: ResultHandler) -> None:
        self._internal.send_cmd(cmd, result_handler, True)

    def send_data(self, data: Data, error_handler: ErrorHandler = None) -> None:
        self._internal.send_data(data, error_handler)

    def send_video_frame(
        self, video_frame: VideoFrame, error_handler: ErrorHandler = None
    ) -> None:
        self._internal.send_video_frame(video_frame, error_handler)

    def send_audio_frame(
        self, audio_frame: AudioFrame, error_handler: ErrorHandler = None
    ) -> None:
        self._internal.send_audio_frame(audio_frame, error_handler)

    def return_result(
        self,
        result: CmdResult,
        target_cmd: Cmd,
        error_handler: ErrorHandler = None,
    ) -> None:
        self._internal.return_result(result, target_cmd, error_handler)

    def return_result_directly(
        self, result: CmdResult, error_handler: ErrorHandler = None
    ) -> None:
        self._internal.return_result_directly(result, error_handler)

    def is_property_exist(self, path: str) -> bool:
        return self._internal.is_property_exist(path)

    def get_property_int(self, path: str) -> int:
        return self._internal.get_property_int(path)

    def set_property_int(self, path: str, value: int) -> None:
        return self._internal.set_property_int(path, value)

    def get_property_string(self, path: str) -> str:
        return self._internal.get_property_string(path)

    def set_property_string(self, path: str, value: str) -> None:
        return self._internal.set_property_string(path, value)

    def get_property_bool(self, path: str) -> bool:
        return self._internal.get_property_bool(path)

    def set_property_bool(self, path: str, value: bool) -> None:
        if value:
            return self._internal.set_property_bool(path, 1)
        else:
            return self._internal.set_property_bool(path, 0)

    def get_property_float(self, path: str) -> float:
        return self._internal.get_property_float(path)

    def set_property_float(self, path: str, value: float) -> None:
        return self._internal.set_property_float(path, value)

    def init_property_from_json(self, json_str: str) -> None:
        return self._internal.init_property_from_json(json_str)
