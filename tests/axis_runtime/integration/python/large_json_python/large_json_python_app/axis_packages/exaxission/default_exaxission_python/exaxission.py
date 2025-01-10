#
# Copyright © 2025 Agora
# This file is part of APTIMA Framework, an open source project.
# Licensed under the Apache License, Version 2.0, with certain conditions.
# Refer to the "LICENSE" file in the root directory for more information.
#
import os
import queue
import threading

from aptima import Cmd, CmdResult, Extension, StatusCode, TenEnv


class DefaultExtension(Extension):
    def __init__(self, name: str) -> None:
        super().__init__(name)
        self.name = name

    def on_configure(self, aptima_env: TenEnv) -> None:
        aptima_env.log_info("on_init")
        assert self.name == "default_extension_python"

        aptima_env.init_property_from_json('{"testKey": "testValue"}')
        aptima_env.on_configure_done()

    def __routine(self, aptima_env: TenEnv):
        self.queue.get()

        i = 0
        for _ in range(0, 10000):
            try:
                throw_exception = False
                _ = aptima_env.get_property_string("undefinedKey")
            except Exception:
                i += 1
                throw_exception = True

            assert throw_exception is True

        self.queue.get()

        print("DefaultExtension __test_thread_routine done")

    def on_start(self, aptima_env: TenEnv) -> None:
        aptima_env.log_debug("on_start")

        aptima_env.set_property_from_json("testKey2", '"testValue2"')
        testValue = aptima_env.get_property_to_json("testKey")
        testValue2 = aptima_env.get_property_to_json("testKey2")
        print("testValue: ", testValue, " testValue2: ", testValue2)

        self.queue = queue.Queue()
        self.thread = threading.Thread(
            target=self.__routine, args=(aptima_env,)
        )
        self.thread.start()

        aptima_env.on_start_done()

    def __join_thread(self, aptima_env: TenEnv):
        self.queue.put(2)

        if self.thread.is_alive():
            self.thread.join()

        aptima_env.on_stop_done()

    def on_stop(self, aptima_env: TenEnv) -> None:
        print("DefaultExtension on_stop")

        # Start a new thread to join the previous thread to avoid blocking the
        # APTIMA extension thread.
        threading.Thread(target=self.__join_thread, args=(aptima_env,)).start()

    def on_deinit(self, aptima_env: TenEnv) -> None:
        print("DefaultExtension on_deinit")
        aptima_env.on_deinit_done()

    def on_cmd(self, aptima_env: TenEnv, cmd: Cmd) -> None:
        print("DefaultExtension on_cmd")

        cmd_json = cmd.get_property_to_json()
        print("DefaultExtension on_cmd json: " + cmd_json)

        cmd_result = CmdResult.create(StatusCode.OK)

        current_path = os.path.dirname(os.path.abspath(__file__))
        json_file = os.path.join(current_path, "test.json")
        print("json_file: ", json_file)

        with open(json_file, "r") as f:
            json = f.read()

        self.queue.put(1)

        cmd_result.set_property_from_json("result", json)

        cmd_result.set_property_string("detail", "ok")

        aptima_env.return_result(cmd_result, cmd)
