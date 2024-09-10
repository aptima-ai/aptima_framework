#
# This file is part of the TEN Framework project.
# See https://github.com/TEN-framework/ten_framework/LICENSE for license
# information.
#
import multiprocessing as mp
import os
import time
from ten import (
    Extension,
    TenEnv,
    Cmd,
    StatusCode,
    CmdResult,
)


class DefaultExtension(Extension):
    def __init__(self, name: str) -> None:
        super().__init__(name)
        self.name = name
        mp.set_start_method("spawn", force=True)
        print("start method", mp.get_start_method())
        self.ready = mp.Value("b", False)

    def on_init(self, ten_env: TenEnv) -> None:
        print("DefaultExtension on_init, name", self.name)
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

        # We set the LD_PRELOAD environment variable to libasan.so so that
        # the subprocess can be run with the AddressSanitizer library.
        os.environ["LD_PRELOAD"] = (
            "ten_packages/system/ten_runtime/lib/libasan.so"
        )

        p = mp.Process(target=f, args=("subproc", self.ready))
        p.start()

        success = True
        count = 0
        while not self.ready.value:
            print("Waiting for server to become ready...")
            count += 1
            if count > 5:
                success = False
                break
            time.sleep(1)
        assert success

        ten_env.send_cmd(
            new_cmd, lambda ten, result: self.check_hello(ten, result, cmd)
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
