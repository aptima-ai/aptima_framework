#
# Copyright © 2024 Agora
# This file is part of TEN Framework, an open source project.
# Licensed under the Apache License, Version 2.0, with certain conditions.
# Refer to the "LICENSE" file in the root directory for more information.
#
import os
import queue
import threading
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

    def on_init(self, ten_env: TenEnv) -> None:
        print("DefaultExtension on_init, name", self.name)
        assert self.name == "default_extension_python"

        ten_env.init_property_from_json('{"testKey": "testValue"}')
        ten_env.on_init_done()

    def __routine(self, ten_env: TenEnv):
        start = self.queue.get()

        i = 0
        for _ in range(0, 10000):
            try:
                throw_exception = False
                _ = ten_env.get_property_string("undefinedKey")
            except Exception:
                i += 1
                throw_exception = True

            assert throw_exception == True

        stop = self.queue.get()

        print("DefaultExtension __test_thread_routine done")

    def on_start(self, ten_env: TenEnv) -> None:
        print("DefaultExtension on_start")

        ten_env.set_property_from_json("testKey2", '"testValue2"')
        testValue = ten_env.get_property_to_json("testKey")
        testValue2 = ten_env.get_property_to_json("testKey2")
        print("testValue: ", testValue, " testValue2: ", testValue2)

        self.queue = queue.Queue()
        self.thread = threading.Thread(target=self.__routine, args=(ten_env,))
        self.thread.start()

        ten_env.on_start_done()

    def __join_thread(self, ten_env: TenEnv):
        self.queue.put(2)

        if self.thread.is_alive():
            self.thread.join()

        ten_env.on_stop_done()

    def on_stop(self, ten_env: TenEnv) -> None:
        print("DefaultExtension on_stop")

        # Start a new thread to join the previous thread to avoid blocking the
        # TEN extension thread.
        threading.Thread(target=self.__join_thread, args=(ten_env,)).start()

    def on_deinit(self, ten_env: TenEnv) -> None:
        print("DefaultExtension on_deinit")
        ten_env.on_deinit_done()

    def on_cmd(self, ten_env: TenEnv, cmd: Cmd) -> None:
        print("DefaultExtension on_cmd")

        cmd_json = cmd.to_json()
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

        ten_env.return_result(cmd_result, cmd)
