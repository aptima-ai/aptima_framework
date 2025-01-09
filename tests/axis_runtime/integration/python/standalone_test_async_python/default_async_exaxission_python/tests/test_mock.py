#
# Copyright © 2025 Agora
# This file is part of TEN Framework, an open source project.
# Licensed under the Apache License, Version 2.0, with certain conditions.
# Refer to the "LICENSE" file in the root directory for more information.
#
from pathlib import Path
from typing import Optional

from ten import (Cmd, CmdResult, ExtensionTester, StatusCode, TenEnvTester,
                 TenError)


class ExtensionTesterMock(ExtensionTester):
    def check_weather(
        self,
        aptima_env: TenEnvTester,
        result: Optional[CmdResult],
        error: Optional[TenError],
    ):
        if error is not None:
            assert False, error

        assert result is not None

        statusCode = result.get_status_code()
        print("receive weather, status:" + str(statusCode))

        if statusCode == StatusCode.OK:
            detail = result.get_property_string("detail")
            assert detail == "sunny"

            aptima_env.stop_test()

    def on_start(self, aptima_env: TenEnvTester) -> None:
        cmd = Cmd.create("query_weather")

        aptima_env.send_cmd(
            cmd,
            lambda aptima_env, result, error: self.check_weather(
                aptima_env, result, error
            ),
        )

        aptima_env.on_start_done()

    def on_cmd(self, aptima_env: TenEnvTester, cmd: Cmd) -> None:
        print("ExtensionTesterMock on_cmd: " + cmd.get_name())

        if cmd.get_name() == "query_weather":
            cmd_result = CmdResult.create(StatusCode.OK)
            cmd_result.set_property_string("detail", "sunny")
            aptima_env.return_result(cmd_result, cmd)


def test_mock_cmd_result():
    tester = ExtensionTesterMock()
    tester.add_addon_base_dir(str(Path(__file__).resolve().parent.parent))
    tester.set_test_mode_single("default_async_extension_python")
    tester.run()


if __name__ == "__main__":
    test_mock_cmd_result()
