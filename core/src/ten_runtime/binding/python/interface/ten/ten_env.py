#
# Copyright © 2024 Agora
# This file is part of TEN Framework, an open source project.
# Licensed under the Apache License, Version 2.0, with certain conditions.
# Refer to the "LICENSE" file in the root directory for more information.
#
from typing import Callable
import inspect
from libten_runtime_python import _Extension, _TenEnv
from .ten_env_attach_to_enum import _TenEnvAttachTo
from .cmd_result import CmdResult
from .cmd import Cmd
from .video_frame import VideoFrame
from .audio_frame import AudioFrame
from .data import Data
from .log_level import LogLevel


class TenEnv: ...  # type: ignore


ResultHandler = Callable[[TenEnv, CmdResult], None] | None


class TenEnv:

    def __init__(self, internal_obj: _TenEnv) -> None:
        self._internal = internal_obj

    def __del__(self) -> None:
        pass

    def _set_release_handler(self, handler: Callable[[], None]) -> None:
        self._release_handler = handler

    def _on_release(self) -> None:
        if hasattr(self, "_release_handler"):
            self._release_handler()

    def on_configure_done(self) -> None:
        from .addon_manager import _AddonManager

        if self._internal._attach_to == _TenEnvAttachTo.APP:
            # Load all python addons when app on_configure_done.
            #
            # In the current use of the TEN framework's Python environment,
            # there is no need to pass any `register_ctx` object into the
            # registration function of the Python addon. Therefore, for now,
            # simply passing `None` is sufficient. If needed in the future, we
            # can consider what information should be passed to the registration
            # function of the Python addon.
            _AddonManager._load_all_addons(None)
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
        return self._internal.send_cmd(cmd, result_handler, False)

    def send_cmd_ex(self, cmd: Cmd, result_handler: ResultHandler) -> None:
        return self._internal.send_cmd(cmd, result_handler, True)

    def send_data(self, data: Data) -> None:
        return self._internal.send_data(data)

    def send_video_frame(self, video_frame: VideoFrame) -> None:
        return self._internal.send_video_frame(video_frame)

    def send_audio_frame(self, audio_frame: AudioFrame) -> None:
        return self._internal.send_audio_frame(audio_frame)

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

    def log_verbose(self, msg: str) -> None:
        self._log_internal(LogLevel.VERBOSE, msg, 2)

    def log_debug(self, msg: str) -> None:
        self._log_internal(LogLevel.DEBUG, msg, 2)

    def log_info(self, msg: str) -> None:
        self._log_internal(LogLevel.INFO, msg, 2)

    def log_warn(self, msg: str) -> None:
        self._log_internal(LogLevel.WARN, msg, 2)

    def log_error(self, msg: str) -> None:
        self._log_internal(LogLevel.ERROR, msg, 2)

    def log_fatal(self, msg: str) -> None:
        self._log_internal(LogLevel.FATAL, msg, 2)

    def log(self, level: LogLevel, msg: str) -> None:
        self._log_internal(level, msg, 1)

    def _log_internal(self, level: LogLevel, msg: str, skip: int) -> None:
        # Get the current frame.
        frame = inspect.currentframe()
        if frame is not None:
            try:
                # Skip the specified number of frames.
                for _ in range(skip):
                    if frame is not None:
                        frame = frame.f_back
                    else:
                        break

                if frame is not None:
                    # Extract information from the caller's frame.
                    file_name = frame.f_code.co_filename
                    func_name = frame.f_code.co_name
                    line_no = frame.f_lineno

                    return self._internal.log(
                        level, func_name, file_name, line_no, msg
                    )
            finally:
                # A defensive programming practice to ensure immediate cleanup
                # of potentially complex reference cycles.
                del frame

        # Fallback in case of failure to get caller information.
        return self._internal.log(level, None, None, 0, msg)
