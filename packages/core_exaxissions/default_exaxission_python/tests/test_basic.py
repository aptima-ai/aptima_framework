#
# Copyright © 2025 Agora
# This file is part of APTIMA Framework, an open source project.
# Licensed under the Apache License, Version 2.0, with certain conditions.
# Refer to the "LICENSE" file in the root directory for more information.
#
from pathlib import Path
from typing import Optional

from aptima import (
    Cmd,
    CmdResult,
    ExtensionTester,
    StatusCode,
    TenEnvTester,
    TenError,
)


class ExtensionTesterBasic(ExtensionTester):
    def check_hello(
        self,
        axis_env: TenEnvTester,
        result: Optional[CmdResult],
        error: Optional[TenError],
    ):
        if error is not None:
            assert False, error.err_msg()

        assert result is not None

        statusCode = result.get_status_code()
        print("receive hello_world, status:" + str(statusCode))

        if statusCode == StatusCode.OK:
            axis_env.stop_test()

    def on_start(self, axis_env: TenEnvTester) -> None:
        new_cmd = Cmd.create("hello_world")

        print("send hello_world")
        axis_env.send_cmd(
            new_cmd,
            lambda axis_env, result, error: self.check_hello(
                axis_env, result, error
            ),
        )

        print("tester on_start_done")
        axis_env.on_start_done()


def test_basic():
    tester = ExtensionTesterBasic()
    tester.add_addon_base_dir(str(Path(__file__).resolve().parent.parent))
    tester.set_test_mode_single("default_extension_python")
    tester.run()
