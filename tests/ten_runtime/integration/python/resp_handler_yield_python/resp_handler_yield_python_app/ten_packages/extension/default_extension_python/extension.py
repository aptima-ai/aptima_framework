#
# Copyright © 2024 Agora
# This file is part of TEN Framework, an open source project.
# Licensed under the Apache License, Version 2.0, with certain conditions.
# Refer to the "LICENSE" file in the root directory for more information.
#
import queue
import threading
from ten import (
    Extension,
    TenEnv,
    Cmd,
)


class DefaultExtension(Extension):
    def __init__(self, name: str) -> None:
        super().__init__(name)
        self.name = name

    def on_configure(self, ten_env: TenEnv) -> None:
        print("DefaultExtension on_init, name", self.name)
        assert self.name == "default_extension_python"

        ten_env.init_property_from_json('{"testKey": "testValue"}')
        ten_env.on_configure_done()

    def on_start(self, ten_env: TenEnv) -> None:
        ten_env.log_debug("on_start")

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

    def echo_cmd_result_generator(self, ten_env: TenEnv, cmd: Cmd):
        print("DefaultExtension send_cmd_yeild")

        q = queue.Queue(maxsize=1)

        def task():
            ten_env.send_cmd(cmd, lambda ten_env, result: q.put(result))

        t = threading.Thread(target=task)
        t.start()

        yield q.get()

    def __handle_cmd(self, ten_env: TenEnv, cmd: Cmd):
        print("DefaultExtension __handle_cmd")

        cmd_hello = Cmd.create("hello")

        generator = self.echo_cmd_result_generator(ten_env, cmd_hello)

        result = next(generator)

        ten_env.return_result(result, cmd)

    def on_cmd(self, ten_env: TenEnv, cmd: Cmd) -> None:
        print("DefaultExtension on_cmd")

        cmd_json = cmd.to_json()
        print("DefaultExtension on_cmd json: " + cmd_json)

        self.thread = threading.Thread(
            target=self.__handle_cmd, args=(ten_env, cmd)
        )
        self.thread.start()
