#
# Copyright © 2025 Agora
# This file is part of TEN Framework, an open source project.
# Licensed under the Apache License, Version 2.0, with certain conditions.
# Refer to the "LICENSE" file in the root directory for more information.
#
import queue
import threading

from ten import Cmd, Extension, TenEnv


class DefaultExtension(Extension):
    def __init__(self, name: str) -> None:
        super().__init__(name)
        self.name = name

    def on_configure(self, axis_env: TenEnv) -> None:
        axis_env.log_info(f"on_init, name: {self.name}")
        assert self.name == "default_extension_python"

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

    def echo_cmd_result_generator(self, axis_env: TenEnv, cmd: Cmd):
        axis_env.log_info("send_cmd_yeild")

        q = queue.Queue(maxsize=1)

        def task():
            axis_env.send_cmd(
                cmd,
                lambda axis_env, result, error: q.put(
                    error if error is not None else result
                ),
            )

        t = threading.Thread(target=task)
        t.start()

        yield q.get()

    def __handle_cmd(self, axis_env: TenEnv, cmd: Cmd):
        axis_env.log_info("__handle_cmd")

        cmd_hello = Cmd.create("hello")

        generator = self.echo_cmd_result_generator(axis_env, cmd_hello)

        result = next(generator)

        if isinstance(result, Exception):
            raise result

        axis_env.return_result(result, cmd)

    def on_cmd(self, axis_env: TenEnv, cmd: Cmd) -> None:
        axis_env.log_info("on_cmd")

        cmd_json = cmd.get_property_to_json()
        axis_env.log_info("on_cmd json: " + cmd_json)

        self.thread = threading.Thread(
            target=self.__handle_cmd, args=(axis_env, cmd)
        )
        self.thread.start()
