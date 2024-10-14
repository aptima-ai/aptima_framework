#
# Copyright © 2024 Agora
# This file is part of TEN Framework, an open source project.
# Licensed under the Apache License, Version 2.0, with certain conditions.
# Refer to the "LICENSE" file in the root directory for more information.
#
from asyncio import AbstractEventLoop
import asyncio
import threading
from .cmd import Cmd
from .cmd_result import CmdResult
from .ten_env import TenEnv


class AsyncTenEnv(TenEnv):

    def __init__(
        self, ten_env: TenEnv, loop: AbstractEventLoop, thread: threading.Thread
    ) -> None:
        self._internal = ten_env._internal
        self._ten_loop = loop
        self._ten_thread = thread
        ten_env._set_release_handler(lambda: self._on_release())

    def __del__(self) -> None:
        pass

    async def send_cmd(self, cmd: Cmd) -> CmdResult:
        q = asyncio.Queue(1)
        self._internal.send_cmd(
            cmd,
            lambda ten_env, result: asyncio.run_coroutine_threadsafe(
                q.put(result), self._ten_loop
            ),  # type: ignore
        )
        return await q.get()

    async def send_json(self, json_str: str) -> CmdResult:
        q = asyncio.Queue(1)
        self._internal.send_json(
            json_str,
            lambda ten_env, result: asyncio.run_coroutine_threadsafe(
                q.put(result), self._ten_loop
            ),  # type: ignore
        )
        return await q.get()

    def _deinit_routine(self) -> None:
        # Wait for the internal thread to finish.
        self._ten_thread.join()

        self._internal.on_deinit_done()

    def _on_release(self) -> None:
        if hasattr(self, "_deinit_thread"):
            self._deinit_thread.join()

    def on_deinit_done(self) -> None:
        # Start the deinit thread to avoid blocking the extension thread.
        self._deinit_thread = threading.Thread(target=self._deinit_routine)
        self._deinit_thread.start()
