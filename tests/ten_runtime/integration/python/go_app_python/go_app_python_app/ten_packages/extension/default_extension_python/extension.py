#
# Copyright © 2024 Agora
# This file is part of TEN Framework, an open source project.
# Licensed under the Apache License, Version 2.0, with certain conditions.
# Refer to the "LICENSE" file in the root directory for more information.
#

# import debugpy
# debugpy.listen(5678)
# debugpy.wait_for_client()

from ten import (
    Extension,
    TenEnv,
    Cmd,
    StatusCode,
    CmdResult,
)


class DefaultExtension(Extension):
    def on_configure(self, ten_env: TenEnv) -> None:
        ten_env.log_debug("on_init")

        ten_env.init_property_from_json('{"testKey": "testValue"}')
        ten_env.on_configure_done()

    def on_start(self, ten_env: TenEnv) -> None:
        ten_env.log_debug("on_start")

        ten_env.set_property_from_json("testKey2", '"testValue2"')
        testValue = ten_env.get_property_to_json("testKey")
        testValue2 = ten_env.get_property_to_json("testKey2")
        ten_env.log_info(f"testValue: {testValue}, testValue2: {testValue2}")

        ten_env.on_start_done()

    def on_stop(self, ten_env: TenEnv) -> None:
        ten_env.log_debug("on_stop")
        ten_env.on_stop_done()

    def on_deinit(self, ten_env: TenEnv) -> None:
        ten_env.log_debug("on_deinit")
        ten_env.on_deinit_done()

    def check_greeting(
        self, ten_env: TenEnv, result: CmdResult, receivedCmd: Cmd
    ):
        statusCode = result.get_status_code()
        detail = result.get_property_string("detail")
        ten_env.log_info(
            f"check_greeting: status: {str(statusCode)}, detail: {detail}"
        )

        respCmd = CmdResult.create(StatusCode.OK)
        respCmd.set_property_string("detail", detail + " nbnb")
        ten_env.log_info("create respCmd")

        ten_env.return_result(respCmd, receivedCmd)

    def check_hello(self, ten_env: TenEnv, result: CmdResult, receivedCmd: Cmd):
        statusCode = result.get_status_code()
        detail = result.get_property_string("detail")
        ten_env.log_info(
            f"check_hello: status: {str(statusCode)}, detail: {detail}"
        )

        # Send a command to go extension.
        new_cmd = Cmd.create("greeting")
        ten_env.send_cmd(
            new_cmd,
            lambda ten_env, result: self.check_greeting(
                ten_env, result, receivedCmd
            ),
        )

    def on_cmd(self, ten_env: TenEnv, cmd: Cmd) -> None:
        cmd_json = cmd.to_json()
        ten_env.log_debug("on_cmd: " + cmd_json)

        new_cmd = Cmd.create("hello")
        new_cmd.set_property_from_json("test", '"testValue2"')
        test_value = new_cmd.get_property_to_json("test")
        ten_env.log_info(f"on_cmd test_value: {test_value}")

        # Send command to a cpp extension.
        ten_env.send_cmd(
            new_cmd,
            lambda ten_env, result: self.check_hello(ten_env, result, cmd),
        )
