#
# This file is part of the TEN Framework project.
# See https://github.com/TEN-framework/ten_framework/LICENSE for license
# information.
#
from typing import Callable
from libten_runtime_python import _Extension, _TenEnv
from .ten_env_attach_to_enum import _TenEnvAttachTo
from .cmd_result import CmdResult
from .cmd import Cmd
from .video_frame import VideoFrame
from .audio_frame import AudioFrame
from .data import Data


class TenEnv: ...  # type: ignore


ResultHandler = Callable[[TenEnv, CmdResult], None] | None


class TenEnv:

    def __init__(self, internal_obj: _TenEnv) -> None:
        self._internal = internal_obj

    def __del__(self) -> None:
        pass

    def on_init_done(self) -> None:
        from .addon import Addon

        if self._internal._attach_to == _TenEnvAttachTo.APP:
            # Load all python addons when app on_init_done.
            Addon._load_all()
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
        return self._internal.send_cmd(cmd, result_handler)

    def send_data(self, data: Data) -> None:
        return self._internal.send_data(data)

    def send_video_frame(self, video_frame: VideoFrame) -> None:
        return self._internal.send_video_frame(video_frame)

    def send_audio_frame(self, audio_frame: AudioFrame) -> None:
        return self._internal.send_audio_frame(audio_frame)

    def send_json(self, json_str: str, result_handler: ResultHandler) -> None:
        return self._internal.send_json(json_str, result_handler)

    def return_result(self, result: CmdResult, target_cmd: Cmd) -> None:
        return self._internal.return_result(result, target_cmd)

    def return_result_directly(self, result: CmdResult) -> None:
        return self._internal.return_result_directly(result)

    def is_property_exist(self, path: str) -> bool:
        return self._internal.is_property_exist(path)

    def is_cmd_connected(self, msg_name: str) -> bool:
        return self._internal.is_cmd_connected(msg_name)

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
