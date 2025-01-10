#
# Copyright © 2025 Agora
# This file is part of APTIMA Framework, an open source project.
# Licensed under the Apache License, Version 2.0, with certain conditions.
# Refer to the "LICENSE" file in the root directory for more information.
#

# import debugpy
# debugpy.listen(5678)
# debugpy.wait_for_client()

from typing import Optional

from aptima import Cmd, CmdResult, Extension, StatusCode, TenEnv, TenError


class DefaultExtension(Extension):
    def on_configure(self, aptima_env: TenEnv) -> None:
        aptima_env.log_debug("on_init")

        aptima_env.init_property_from_json('{"testKey": "testValue"}')
        aptima_env.on_configure_done()

    def on_start(self, aptima_env: TenEnv) -> None:
        aptima_env.log_debug("on_start")

        aptima_env.set_property_from_json("testKey2", '"testValue2"')
        testValue = aptima_env.get_property_to_json("testKey")
        testValue2 = aptima_env.get_property_to_json("testKey2")
        aptima_env.log_info(f"testValue: {testValue}, testValue2: {testValue2}")

        aptima_env.on_start_done()

    def on_stop(self, aptima_env: TenEnv) -> None:
        aptima_env.log_debug("on_stop")
        aptima_env.on_stop_done()

    def on_deinit(self, aptima_env: TenEnv) -> None:
        aptima_env.log_debug("on_deinit")
        aptima_env.on_deinit_done()

    def check_greeting(
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
        aptima_env.log_info(f"check_greeting: status: {str(statusCode)}")

        respCmd = CmdResult.create(StatusCode.OK)
        respCmd.set_property_string("detail", "received response")
        aptima_env.log_info("create respCmd")

        aptima_env.return_result(respCmd, receivedCmd)

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
            f"check_hello: status: {str(statusCode)}, detail: {detail}"
        )

        # Send a command to go extension.
        new_cmd = Cmd.create("greeting")
        aptima_env.send_cmd(
            new_cmd,
            lambda aptima_env, result, error: self.check_greeting(
                aptima_env, result, error, receivedCmd
            ),
        )

    def on_cmd(self, aptima_env: TenEnv, cmd: Cmd) -> None:
        cmd_json = cmd.get_property_to_json()
        aptima_env.log_debug("on_cmd: " + cmd_json)

        new_cmd = Cmd.create("hello")
        new_cmd.set_property_from_json("test", '"testValue2"')
        test_value = new_cmd.get_property_to_json("test")
        aptima_env.log_info(f"on_cmd test_value: {test_value}")

        # Send command to a cpp extension.
        aptima_env.send_cmd(
            new_cmd,
            lambda aptima_env, result, error: self.check_hello(
                aptima_env, result, error, cmd
            ),
        )
