#
# Copyright © 2024 Agora
# This file is part of TEN Framework, an open source project.
# Licensed under the Apache License, Version 2.0, with certain conditions.
# Refer to the "LICENSE" file in the root directory for more information.
#
import threading
import asyncio
from ten import (
    Extension,
    TenEnv,
    Cmd,
    CmdResult,
)


class DefaultExtension(Extension):
    async def __thread_routine(self, ten_env: TenEnv):
        print("DefaultExtension __thread_routine start")

        self.loop = asyncio.get_running_loop()

        # Mock async operation before on_start_done
        await asyncio.sleep(1)

        ten_env.on_start_done()

        # Suspend the thread until stopEvent is set
        await self.stopEvent.wait()

    async def stop_thread(self):
        self.stopEvent.set()

    async def send_cmd_async(self, ten_env: TenEnv, cmd: Cmd) -> CmdResult:
        print("DefaultExtension send_cmd_async")
        q = asyncio.Queue(1)
        ten_env.send_cmd(
            cmd,
            lambda ten_env, result: asyncio.run_coroutine_threadsafe(
                q.put(result), self.loop
            ),  # type: ignore
        )
        return await q.get()

    def __init__(self, name: str) -> None:
        super().__init__(name)
        self.name = name
        self.stopEvent = asyncio.Event()

    def on_init(self, ten_env: TenEnv) -> None:
        ten_env.on_init_done()

    def on_start(self, ten_env: TenEnv) -> None:
        ten_env.log_debug("on_start")

        self.thread = threading.Thread(
            target=asyncio.run, args=(self.__thread_routine(ten_env),)
        )

        # Then 'on_start_done' will be called in the thread
        self.thread.start()

    def on_deinit(self, ten_env: TenEnv) -> None:
        print("DefaultExtension on_deinit")
        ten_env.on_deinit_done()

    async def on_cmd_async(self, ten_env: TenEnv, cmd: Cmd) -> None:
        print("DefaultExtension on_cmd_async")

        cmd_json = cmd.to_json()
        print("DefaultExtension on_cmd_async json: " + cmd_json)

        # Mock async operation, e.g. network, file I/O
        await asyncio.sleep(1)

        # Send a new command to other extensions and wait for the result. The
        # result will be returned to the original sender.
        new_cmd = Cmd.create("hello")
        cmd_result = await self.send_cmd_async(ten_env, new_cmd)
        ten_env.return_result(cmd_result, cmd)

    def on_cmd(self, ten_env: TenEnv, cmd: Cmd) -> None:
        print("DefaultExtension on_cmd")

        asyncio.run_coroutine_threadsafe(
            self.on_cmd_async(ten_env, cmd), self.loop
        )

    def on_stop(self, ten_env: TenEnv) -> None:
        print("DefaultExtension on_stop")

        if self.thread.is_alive():
            asyncio.run_coroutine_threadsafe(self.stop_thread(), self.loop)
            self.thread.join()

        ten_env.on_stop_done()
