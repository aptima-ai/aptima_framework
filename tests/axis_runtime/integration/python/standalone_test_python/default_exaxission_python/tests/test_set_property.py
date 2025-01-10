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


class ExtensionTesterSetProperty(ExtensionTester):
    def check_greeting(
        self,
        aptima_env: TenEnvTester,
        result: Optional[CmdResult],
        error: Optional[TenError],
    ):
        if error is not None:
            assert False, error

        assert result is not None

        statusCode = result.get_status_code()
        print("receive hello_world, status:" + str(statusCode))

        if statusCode == StatusCode.OK:
            detail = result.get_property_string("detail")
            assert detail == "hola"

            aptima_env.stop_test()

    def on_start(self, aptima_env: TenEnvTester) -> None:
        cmd = Cmd.create("greeting")

        aptima_env.send_cmd(
            cmd,
            lambda aptima_env, result, error: self.check_greeting(
                aptima_env, result, error
            ),
        )

        print("tester on_start_done")
        aptima_env.on_start_done()


def test_set_property():
    tester = ExtensionTesterSetProperty()
    tester.add_addon_base_dir(str(Path(__file__).resolve().parent.parent))
    tester.set_test_mode_single(
        "default_extension_python", '{"greeting": "hola"}'
    )
    tester.run()


if __name__ == "__main__":
    test_set_property()
