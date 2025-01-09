#
# Copyright © 2025 Agora
# This file is part of TEN Framework, an open source project.
# Licensed under the Apache License, Version 2.0, with certain conditions.
# Refer to the "LICENSE" file in the root directory for more information.
#
from typing import Optional

from ten import Cmd, CmdResult, Extension, StatusCode, TenEnv, TenError


class DefaultExtension(Extension):
    def on_configure(self, aptima_env: TenEnv) -> None:
        aptima_env.log_debug("on_init")

        aptima_env.init_property_from_json('{"testKey": "testValue"}')

        aptima_env.on_configure_done()

    def on_start(self, aptima_env: TenEnv) -> None:
        aptima_env.log_debug("on_start")

        # If the io encoding is not utf-8, the following print will cause an
        # error. Ex: UnicodeEncodeError: 'ascii' codec can't encode characters
        # in position 0-1: ordinal not in range(128).
        aptima_env.log_info("中文")

        aptima_env.set_property_from_json("testKey2", '"testValue2"')
        testValue = aptima_env.get_property_to_json("testKey")
        testValue2 = aptima_env.get_property_to_json("testKey2")
        aptima_env.log_info(f"testValue: {testValue}, testValue2: {testValue2}")

        aptima_env.on_start_done()

    def on_stop(self, aptima_env: TenEnv) -> None:
        aptima_env.log_info("on_stop")
        aptima_env.on_stop_done()

    def on_deinit(self, aptima_env: TenEnv) -> None:
        aptima_env.log_info("on_deinit")
        aptima_env.on_deinit_done()

    def check_hello(
        self,
        aptima_env: TenEnv,
        result: Optional[CmdResult],
        error: Optional[TenError],
        receivedCmd: Cmd,
    ):
        if error is not None:
            assert False, error.err_msg()

        assert result is not None

        statusCode = result.get_status_code()
        detail = result.get_property_string("detail")
        aptima_env.log_info(
            "DefaultExtension check_hello: status:"
            + str(statusCode)
            + " detail:"
            + detail
        )

        respCmd = CmdResult.create(StatusCode.OK)
        respCmd.set_property_string("detail", detail + " nbnb")
        aptima_env.log_info("create respCmd")

        aptima_env.return_result(respCmd, receivedCmd)

    def on_cmd(self, aptima_env: TenEnv, cmd: Cmd) -> None:
        aptima_env.log_info("on_cmd")

        cmd_json = cmd.get_property_to_json()
        aptima_env.log_info("on_cmd json: " + cmd_json)

        new_cmd = Cmd.create("hello")
        new_cmd.set_property_from_json("test", '"testValue2"')
        test_value = new_cmd.get_property_to_json("test")
        aptima_env.log_info("on_cmd test_value: " + test_value)

        aptima_env.send_cmd(
            new_cmd,
            lambda aptima_env, result, error: self.check_hello(
                aptima_env, result, error, cmd
            ),
        )
