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
from typing import AsyncGenerator


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

    async def send_cmd(self, cmd: Cmd) -> AsyncGenerator[CmdResult, None]:
        q = asyncio.Queue(maxsize=10)
        self._internal.send_cmd(
            cmd,
            lambda _, result: asyncio.run_coroutine_threadsafe(
                q.put(result), self._ten_loop
            ),
        )

        while True:
            result: CmdResult = await q.get()
            if result.is_completed():
                yield result
                # This is the final result, so break the while loop.
                break
            yield result

    async def send_json(self, json_str: str) -> AsyncGenerator[CmdResult, None]:
        q = asyncio.Queue(maxsize=10)
        self._internal.send_json(
            json_str,
            lambda ten_env, result: asyncio.run_coroutine_threadsafe(
                q.put(result), self._ten_loop
            ),
        )
        while True:
            result: CmdResult = await q.get()
            if result.is_completed():
                yield result
                # This is the final result, so break the while loop.
                break
            yield result

    def _deinit_routine(self) -> None:
        # Wait for the internal thread to finish.
        self._ten_thread.join()

        self._internal.on_deinit_done()

    def _on_release(self) -> None:
        if hasattr(self, "_deinit_thread"):
            self._deinit_thread.join()

    def _deinit(self) -> None:
        # Start the deinit thread to avoid blocking the extension thread.
        self._deinit_thread = threading.Thread(target=self._deinit_routine)
        self._deinit_thread.start()

    def on_configure_done(self) -> None:
        raise NotImplementedError(
            "No need to call this method in async extension"
        )

    def on_init_done(self) -> None:
        raise NotImplementedError(
            "No need to call this method in async extension"
        )

    def on_start_done(self) -> None:
        raise NotImplementedError(
            "No need to call this method in async extension"
        )

    def on_stop_done(self) -> None:
        raise NotImplementedError(
            "No need to call this method in async extension"
        )

    def on_deinit_done(self) -> None:
        raise NotImplementedError(
            "No need to call this method in async extension"
        )
