#
# Copyright © 2025 Agora
# This file is part of APTIMA Framework, an open source project.
# Licensed under the Apache License, Version 2.0, with certain conditions.
# Refer to the "LICENSE" file in the root directory for more information.
#
import asyncio
import threading

from aptima import Cmd, CmdResult, Extension, TenEnv


class DefaultExtension(Extension):
    async def __thread_routine(self, aptima_env: TenEnv):
        aptima_env.log_info("__thread_routine start")

        self.loop = asyncio.get_running_loop()

        # Mock async operation before on_start_done
        await asyncio.sleep(1)

        aptima_env.on_start_done()

        # Suspend the thread until stopEvent is set
        await self.stopEvent.wait()

    async def stop_thread(self):
        self.stopEvent.set()

    async def send_cmd_async(self, aptima_env: TenEnv, cmd: Cmd) -> CmdResult:
        aptima_env.log_info("send_cmd_async")
        q = asyncio.Queue(maxsize=10)
        aptima_env.send_cmd(
            cmd,
            lambda aptima_env, result, error: asyncio.run_coroutine_threadsafe(
                q.put([result, error]), self.loop
            ),  # type: ignore
        )

        [result, error] = await q.get()
        if error is not None:
            raise Exception(error.err_msg())
        return result

    def __init__(self, name: str) -> None:
        super().__init__(name)
        self.name = name
        self.stopEvent = asyncio.Event()

    def on_init(self, aptima_env: TenEnv) -> None:
        aptima_env.on_init_done()

    def on_start(self, aptima_env: TenEnv) -> None:
        aptima_env.log_debug("on_start")

        self.thread = threading.Thread(
            target=asyncio.run, args=(self.__thread_routine(aptima_env),)
        )

        # Then 'on_start_done' will be called in the thread
        self.thread.start()

    def on_deinit(self, aptima_env: TenEnv) -> None:
        aptima_env.log_info("on_deinit")
        aptima_env.on_deinit_done()

    async def on_cmd_async(self, aptima_env: TenEnv, cmd: Cmd) -> None:
        aptima_env.log_info("on_cmd_async")

        cmd_json = cmd.get_property_to_json()
        aptima_env.log_info("on_cmd_async json: " + cmd_json)

        # Mock async operation, e.g. network, file I/O
        await asyncio.sleep(1)

        # Send a new command to other extensions and wait for the result. The
        # result will be returned to the original sender.
        new_cmd = Cmd.create("hello")
        cmd_result = await self.send_cmd_async(aptima_env, new_cmd)
        aptima_env.return_result(cmd_result, cmd)

    def on_cmd(self, aptima_env: TenEnv, cmd: Cmd) -> None:
        aptima_env.log_info("on_cmd")

        asyncio.run_coroutine_threadsafe(
            self.on_cmd_async(aptima_env, cmd), self.loop
        )

    def on_stop(self, aptima_env: TenEnv) -> None:
        aptima_env.log_info("on_stop")

        if self.thread.is_alive():
            asyncio.run_coroutine_threadsafe(self.stop_thread(), self.loop)
            self.thread.join()

        aptima_env.on_stop_done()
