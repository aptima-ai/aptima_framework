#
# Copyright © 2024 Agora
# This file is part of TEN Framework, an open source project.
# Licensed under the Apache License, Version 2.0, with certain conditions.
# Refer to the "LICENSE" file in the root directory for more information.
#
from ten import Extension, TenEnv, Cmd, StatusCode, CmdResult, LogLevel


class DefaultExtension(Extension):
    def __init__(self, name: str) -> None:
        super().__init__(name)
        self.name = name

    def on_init(self, ten_env: TenEnv) -> None:
        ten_env.log(
            LogLevel.DEBUG, f"DefaultExtension on_init, name: {self.name}"
        )
        assert self.name == "default_extension_python"

        ten_env.init_property_from_json('{"testKey": "testValue"}')
        ten_env.on_init_done()

    def on_start(self, ten_env: TenEnv) -> None:
        print("DefaultExtension on_start")

        ten_env.set_property_from_json("testKey2", '"testValue2"')
        testValue = ten_env.get_property_to_json("testKey")
        testValue2 = ten_env.get_property_to_json("testKey2")
        print("testValue: ", testValue, " testValue2: ", testValue2)

        ten_env.on_start_done()

    def on_stop(self, ten_env: TenEnv) -> None:
        print("DefaultExtension on_stop")
        ten_env.on_stop_done()

    def on_deinit(self, ten_env: TenEnv) -> None:
        print("DefaultExtension on_deinit")
        ten_env.on_deinit_done()

    def check_hello(self, ten_env: TenEnv, result: CmdResult, receivedCmd: Cmd):
        statusCode = result.get_status_code()
        detail = result.get_property_string("detail")
        print(
            "DefaultExtension check_hello: status:"
            + str(statusCode)
            + " detail:"
            + detail
        )

        respCmd = CmdResult.create(StatusCode.OK)
        respCmd.set_property_string("detail", detail + " nbnb")
        print("DefaultExtension create respCmd")

        ten_env.return_result(respCmd, receivedCmd)

    def on_cmd(self, ten_env: TenEnv, cmd: Cmd) -> None:
        print("DefaultExtension on_cmd")

        cmd_json = cmd.to_json()
        print("DefaultExtension on_cmd json: " + cmd_json)

        new_cmd = Cmd.create("hello")
        new_cmd.set_property_from_json("test", '"testValue2"')
        test_value = new_cmd.get_property_to_json("test")
        print("DefaultExtension on_cmd test_value: " + test_value)

        ten_env.send_cmd(
            new_cmd, lambda ten, result: self.check_hello(ten, result, cmd)
        )
