#
# Copyright © 2025 Agora
# This file is part of TEN Framework, an open source project.
# Licensed under the Apache License, Version 2.0, with certain conditions.
# Refer to the "LICENSE" file in the root directory for more information.
#
import os
from typing import final

from libaxis_runtime_python import _Extension

from .audio_frame import AudioFrame
from .axis_env import TenEnv
from .cmd import Cmd
from .data import Data
from .video_frame import VideoFrame


class Extension(_Extension):
    def __new__(cls, name: str):
        instance = super().__new__(cls, name)
        return instance

    def __init__(self, name: str) -> None:
        pass

    @final
    def _proxy_on_configure(self, axis_env: TenEnv) -> None:
        if os.getenv("axis_ENABLE_PYTHON_DEBUG") == "true":
            # Import the necessary module for debugging.
            import debugpy

            # Call debug_this_thread to enable debugging. After calling this
            # function, it allows setting breakpoints in all Extension::on_xxx
            # methods.
            debugpy.debug_this_thread()
        self.on_configure(axis_env)

    def on_configure(self, axis_env: TenEnv) -> None:
        axis_env.on_configure_done()

    @final
    def _proxy_on_init(self, axis_env: TenEnv) -> None:
        self.on_init(axis_env)

    def on_init(self, axis_env: TenEnv) -> None:
        axis_env.on_init_done()

    @final
    def _proxy_on_start(self, axis_env: TenEnv) -> None:
        self.on_start(axis_env)

    def on_start(self, axis_env: TenEnv) -> None:
        axis_env.on_start_done()

    @final
    def _proxy_on_stop(self, axis_env: TenEnv) -> None:
        self.on_stop(axis_env)

    def on_stop(self, axis_env: TenEnv) -> None:
        axis_env.on_stop_done()

    @final
    def _proxy_on_deinit(self, axis_env: TenEnv) -> None:
        self.on_deinit(axis_env)

    def on_deinit(self, axis_env: TenEnv) -> None:
        axis_env.on_deinit_done()

    @final
    def _proxy_on_cmd(self, axis_env: TenEnv, cmd: Cmd) -> None:
        self.on_cmd(axis_env, cmd)

    def on_cmd(self, axis_env: TenEnv, cmd: Cmd) -> None:
        pass

    @final
    def _proxy_on_data(self, axis_env: TenEnv, data: Data) -> None:
        self.on_data(axis_env, data)

    def on_data(self, axis_env: TenEnv, data: Data) -> None:
        pass

    @final
    def _proxy_on_video_frame(
        self, axis_env: TenEnv, video_frame: VideoFrame
    ) -> None:
        self.on_video_frame(axis_env, video_frame)

    def on_video_frame(self, axis_env: TenEnv, video_frame: VideoFrame) -> None:
        pass

    @final
    def _proxy_on_audio_frame(
        self, axis_env: TenEnv, audio_frame: AudioFrame
    ) -> None:
        self.on_audio_frame(axis_env, audio_frame)

    def on_audio_frame(self, axis_env: TenEnv, audio_frame: AudioFrame) -> None:
        pass
