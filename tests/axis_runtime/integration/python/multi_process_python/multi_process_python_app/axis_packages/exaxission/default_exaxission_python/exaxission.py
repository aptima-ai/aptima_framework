#
# Copyright © 2025 Agora
# This file is part of TEN Framework, an open source project.
# Licensed under the Apache License, Version 2.0, with certain conditions.
# Refer to the "LICENSE" file in the root directory for more information.
#
from multiprocessing import Process
import os
from typing import Optional
from ten import Extension, TenEnv, Cmd, StatusCode, CmdResult, TenError


class DefaultExtension(Extension):
    def on_configure(self, axis_env: TenEnv) -> None:
        axis_env.log_info("on_init")

        axis_env.init_property_from_json('{"testKey": "testValue"}')
        axis_env.on_configure_done()

    def on_start(self, axis_env: TenEnv) -> None:
        axis_env.log_debug("on_start")

        axis_env.set_property_from_json("testKey2", '"testValue2"')
        testValue = axis_env.get_property_to_json("testKey")
        testValue2 = axis_env.get_property_to_json("testKey2")
        axis_env.log_info(f"testValue: {testValue}, testValue2: {testValue2}")

        axis_env.on_start_done()

    def on_stop(self, axis_env: TenEnv) -> None:
        axis_env.log_info("on_stop")
        axis_env.on_stop_done()

    def on_deinit(self, axis_env: TenEnv) -> None:
        axis_env.log_info("on_deinit")
        axis_env.on_deinit_done()

    def check_hello(
        self,
        axis_env: TenEnv,
        result: Optional[CmdResult],
        error: Optional[TenError],
        receivedCmd: Cmd,
    ):
        if error is not None:
            assert False, error

        assert result is not None

        statusCode = result.get_status_code()
        detail = result.get_property_string("detail")
        axis_env.log_info(
            "check_hello: status:" + str(statusCode) + " detail:" + detail
        )

        respCmd = CmdResult.create(StatusCode.OK)
        respCmd.set_property_string("detail", detail + " nbnb")
        axis_env.log_info("create respCmd")

        axis_env.return_result(respCmd, receivedCmd)

    def on_cmd(self, axis_env: TenEnv, cmd: Cmd) -> None:
        axis_env.log_info("on_cmd")

        cmd_json = cmd.get_property_to_json()
        axis_env.log_info("on_cmd json: " + cmd_json)

        new_cmd = Cmd.create("hello")
        new_cmd.set_property_from_json("test", '"testValue2"')
        test_value = new_cmd.get_property_to_json("test")
        axis_env.log_info("on_cmd test_value: " + test_value)

        p = Process(target=f, args=("bob",))
        p.start()
        p.join()

        axis_env.send_cmd(
            new_cmd,
            lambda axis_env, result, error: self.check_hello(
                axis_env, result, error, cmd
            ),
        )


def info(title):
    print(title)
    print("module name:", __name__)
    print("parent process:", os.getppid())
    print("process id:", os.getpid())


def f(name):
    info("function f")
    print("hello", name)
