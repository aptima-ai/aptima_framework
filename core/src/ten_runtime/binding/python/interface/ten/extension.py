#
# This file is part of the TEN Framework project.
# See https://github.com/TEN-framework/ten_framework/LICENSE for license
# information.
#
import os
from typing import final
from libten_runtime_python import _Extension
from .video_frame import VideoFrame
from .audio_frame import AudioFrame
from .ten_env import TenEnv
from .cmd import Cmd
from .data import Data


class Extension(_Extension):
    def __init__(self, name: str) -> None:
        pass

    @final
    def _proxy_on_init(self, ten_env: TenEnv) -> None:
        if os.getenv("TEN_ENABLE_PYTHON_DEBUG") == "true":
            # Import the necessary module for debugging.
            import debugpy

            # Call debug_this_thread to enable debugging. After calling this
            # function, it allows setting breakpoints in all Extension::on_xxx
            # methods.
            debugpy.debug_this_thread()
        self.on_init(ten_env)

    def on_init(self, ten_env: TenEnv) -> None:
        ten_env.on_init_done()

    def on_start(self, ten_env: TenEnv) -> None:
        ten_env.on_start_done()

    def on_stop(self, ten_env: TenEnv) -> None:
        ten_env.on_stop_done()

    def on_deinit(self, ten_env: TenEnv) -> None:
        ten_env.on_deinit_done()

    def on_cmd(self, ten_env: TenEnv, cmd: Cmd) -> None:
        pass

    def on_data(self, ten_env: TenEnv, data: Data) -> None:
        pass

    def on_video_frame(self, ten_env: TenEnv, video_frame: VideoFrame) -> None:
        pass

    def on_audio_frame(self, ten_env: TenEnv, audio_frame: AudioFrame) -> None:
        pass
