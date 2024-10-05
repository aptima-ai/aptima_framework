#
# Copyright © 2024 Agora
# This file is part of TEN Framework, an open source project.
# Licensed under the Apache License, Version 2.0, with certain conditions.
# Refer to the "LICENSE" file in the root directory for more information.
#
from typing import Callable, final
from libten_runtime_python import _ExtensionTester, _TenEnvTester
from .cmd import Cmd
from .cmd_result import CmdResult


class TenEnvTester: ...  # type: ignore


ResultHandler = Callable[[TenEnvTester, CmdResult], None] | None


class TenEnvTester:

    def __init__(self, internal_obj: _TenEnvTester) -> None:
        self._internal = internal_obj

    def __del__(self) -> None:
        pass

    def on_start_done(self) -> None:
        return self._internal.on_start_done()

    def send_cmd(self, cmd: Cmd, result_handler: ResultHandler) -> None:
        return self._internal.send_cmd(cmd, result_handler)

    def stop_test(self) -> None:
        return self._internal.stop_test()


class ExtensionTester(_ExtensionTester):
    @final
    def add_addon_base_dir(self, base_dir: str) -> None:
        return _ExtensionTester.add_addon_base_dir(self, base_dir)

    @final
    def set_test_mode_single(self, addon_name: str) -> None:
        return _ExtensionTester.set_test_mode_single(self, addon_name)

    @final
    def run(self) -> None:
        return _ExtensionTester.run(self)

    def on_start(self, ten_env_tester: TenEnvTester) -> None:
        ten_env_tester.on_start_done()

    def on_cmd(self, ten_env_tester: TenEnvTester, cmd: Cmd) -> None:
        pass
