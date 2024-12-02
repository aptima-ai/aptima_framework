#
# Copyright © 2024 Agora
# This file is part of TEN Framework, an open source project.
# Licensed under the Apache License, Version 2.0, with certain conditions.
# Refer to the "LICENSE" file in the root directory for more information.
#
import sys
import importlib
from pathlib import Path
from typing import Callable, Optional, final

from libten_runtime_python import _ExtensionTester, _TenEnvTester
from .error import TenError
from .cmd import Cmd
from .data import Data
from .audio_frame import AudioFrame
from .video_frame import VideoFrame
from .cmd_result import CmdResult
from .addon_manager import _AddonManager


class TenEnvTester: ...  # type: ignore


ResultHandler = (
    Callable[[TenEnvTester, Optional[CmdResult], Optional[TenError]], None]
    | None
)


class TenEnvTester:

    def __init__(self, internal_obj: _TenEnvTester) -> None:
        self._internal = internal_obj

    def __del__(self) -> None:
        pass

    def on_start_done(self) -> None:
        return self._internal.on_start_done()

    def send_cmd(self, cmd: Cmd, result_handler: ResultHandler) -> None:
        return self._internal.send_cmd(cmd, result_handler)

    def send_data(self, data: Data) -> None:
        return self._internal.send_data(data)

    def send_audio_frame(self, audio_frame: AudioFrame) -> None:
        return self._internal.send_audio_frame(audio_frame)

    def send_video_frame(self, video_frame: VideoFrame) -> None:
        return self._internal.send_video_frame(video_frame)

    def stop_test(self) -> None:
        return self._internal.stop_test()


class ExtensionTester(_ExtensionTester):
    def __init__(self):
        self.addon_base_dirs = []

    @final
    def _on_test_app_configure(self, ten_env_tester: TenEnvTester) -> None:
        self.on_start(ten_env_tester)

    @final
    def _import_package_from_path(self, addon_base_dir_str: str) -> None:
        addon_base_dir = Path(addon_base_dir_str).resolve()
        if str(addon_base_dir.parent) not in sys.path:
            sys.path.insert(0, str(addon_base_dir.parent))
        importlib.import_module(addon_base_dir.name)

        # TODO(Wei): This should be done during the `on_configure_done` of the
        # test app.
        _AddonManager.register_all_addons(None)

    @final
    def add_addon_base_dir(self, base_dir: str) -> None:
        self.addon_base_dirs.append(base_dir)

    @final
    def set_test_mode_single(self, addon_name: str) -> None:
        return _ExtensionTester.set_test_mode_single(self, addon_name)

    @final
    def run(self) -> None:
        # Import the addon packages.
        for addon_base_dir in self.addon_base_dirs:
            self._import_package_from_path(addon_base_dir)

        return _ExtensionTester.run(self)

    def on_start(self, ten_env_tester: TenEnvTester) -> None:
        ten_env_tester.on_start_done()

    def on_cmd(self, ten_env_tester: TenEnvTester, cmd: Cmd) -> None:
        pass

    def on_data(self, ten_env_tester: TenEnvTester, data: Data) -> None:
        pass

    def on_audio_frame(
        self, ten_env_tester: TenEnvTester, audio_frame: AudioFrame
    ) -> None:
        pass

    def on_video_frame(
        self, ten_env_tester: TenEnvTester, video_frame: VideoFrame
    ) -> None:
        pass
