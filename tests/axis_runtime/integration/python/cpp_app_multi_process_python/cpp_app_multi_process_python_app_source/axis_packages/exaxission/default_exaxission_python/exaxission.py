#
# Copyright © 2025 Agora
# This file is part of APTIMA Framework, an open source project.
# Licensed under the Apache License, Version 2.0, with certain conditions.
# Refer to the "LICENSE" file in the root directory for more information.
#
import multiprocessing as mp
import os
import time
from typing import Optional

from aptima import Cmd, CmdResult, Extension, StatusCode, TenEnv, TenError


class DefaultExtension(Extension):
    def __init__(self, name: str) -> None:
        super().__init__(name)
        self.name = name

        mp.set_start_method("spawn", force=True)
        print("start method", mp.get_start_method())
        self.ready = mp.Value("b", False)

    def on_configure(self, aptima_env: TenEnv) -> None:
        aptima_env.log_debug("on_init")
        assert self.name == "default_extension_python"

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
            "check_hello: status:" + str(statusCode) + " detail:" + detail
        )

        respCmd = CmdResult.create(StatusCode.OK)
        respCmd.set_property_string("detail", detail + " nbnb")
        aptima_env.log_info("create respCmd")

        aptima_env.return_result(respCmd, receivedCmd)

    def on_cmd(self, aptima_env: TenEnv, cmd: Cmd) -> None:
        cmd_json = cmd.get_property_to_json()
        aptima_env.log_debug("on_cmd json: " + cmd_json)

        new_cmd = Cmd.create("hello")
        new_cmd.set_property_from_json("test", '"testValue2"')
        test_value = new_cmd.get_property_to_json("test")
        aptima_env.log_debug(f"on_cmd test_value: {test_value}")

        # We set the LD_PRELOAD environment variable to libasan.so so that
        # the subprocess can be run with the AddressSanitizer library.
        os.environ["LD_PRELOAD"] = (
            "aptima_packages/system/aptima_runtime/lib/libasan.so"
        )

        p = mp.Process(target=f, args=("subproc", self.ready))
        p.start()

        success = True
        count = 0
        while not self.ready.value:
            aptima_env.log_debug("Waiting for server to become ready...")
            count += 1
            if count > 5:
                success = False
                break
            time.sleep(1)
        assert success

        aptima_env.send_cmd(
            new_cmd,
            lambda aptima_env, result, error: self.check_hello(
                aptima_env, result, error, cmd
            ),
        )


def info(title):
    print(title)
    print("module name:", __name__)
    print("parent process:", os.getppid())
    print("process id:", os.getpid())


def f(name, ready):
    info("function f")
    print("hello", name)
    ready.value = True
