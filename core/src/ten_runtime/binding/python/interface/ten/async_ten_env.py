#
# Copyright © 2024 Agora
# This file is part of TEN Framework, an open source project.
# Licensed under the Apache License, Version 2.0, with certain conditions.
# Refer to the "LICENSE" file in the root directory for more information.
#
import asyncio
import threading
from asyncio import AbstractEventLoop
from typing import AsyncGenerator

from .cmd import Cmd
from .data import Data
from .video_frame import VideoFrame
from .audio_frame import AudioFrame
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
        q = asyncio.Queue(maxsize=1)
        self._internal.send_cmd(
            cmd,
            lambda _, result, error: asyncio.run_coroutine_threadsafe(
                q.put([result, error]),
                self._ten_loop,
            ),  # type: ignore
            False,
        )

        [result, error] = await q.get()
        if error is not None:
            raise Exception(error.err_msg())

        assert result.is_completed()
        return result

    async def send_cmd_ex(self, cmd: Cmd) -> AsyncGenerator[CmdResult, None]:
        q = asyncio.Queue(maxsize=10)
        self._internal.send_cmd(
            cmd,
            lambda _, result, error: asyncio.run_coroutine_threadsafe(
                q.put([result, error]),
                self._ten_loop,
            ),  # type: ignore
            True,
        )

        while True:
            [result, error] = await q.get()
            if error is not None:
                raise Exception(error.err_msg())
            else:
                if result.is_completed():
                    yield result
                    # This is the final result, so break the while loop.
                    break
                yield result

    async def send_data(self, data: Data) -> None:
        q = asyncio.Queue(maxsize=1)
        self._internal.send_data(
            data,
            lambda _, error: asyncio.run_coroutine_threadsafe(
                q.put(error),
                self._ten_loop,
            ),  # type: ignore
        )

        error = await q.get()
        if error is not None:
            raise Exception(error.err_msg())

    async def send_video_frame(self, video_frame: VideoFrame) -> None:
        q = asyncio.Queue(maxsize=1)
        self._internal.send_video_frame(
            video_frame,
            lambda _, error: asyncio.run_coroutine_threadsafe(
                q.put(error),
                self._ten_loop,
            ),  # type: ignore
        )

        error = await q.get()
        if error is not None:
            raise Exception(error.err_msg())

    async def send_audio_frame(self, audio_frame: AudioFrame) -> None:
        q = asyncio.Queue(maxsize=1)
        self._internal.send_audio_frame(
            audio_frame,
            lambda _, error: asyncio.run_coroutine_threadsafe(
                q.put(error),
                self._ten_loop,
            ),  # type: ignore
        )

        error = await q.get()
        if error is not None:
            raise Exception(error.err_msg())

    async def return_result(self, result: CmdResult, target_cmd: Cmd) -> None:
        q = asyncio.Queue(maxsize=1)
        self._internal.return_result(
            result,
            target_cmd,
            lambda _, error: asyncio.run_coroutine_threadsafe(
                q.put(error),
                self._ten_loop,
            ),  # type: ignore
        )

        error = await q.get()
        if error is not None:
            raise Exception(error.err_msg())

    async def return_result_directly(self, result: CmdResult) -> None:
        q = asyncio.Queue(maxsize=1)
        self._internal.return_result_directly(
            result,
            lambda _, error: asyncio.run_coroutine_threadsafe(
                q.put(error),
                self._ten_loop,
            ),  # type: ignore
        )

        error = await q.get()
        if error is not None:
            raise Exception(error.err_msg())

    async def on_configure_done(self) -> None:
        raise NotImplementedError(
            "No need to call this method in async extension"
        )

    async def on_init_done(self) -> None:
        raise NotImplementedError(
            "No need to call this method in async extension"
        )

    async def on_start_done(self) -> None:
        raise NotImplementedError(
            "No need to call this method in async extension"
        )

    async def on_stop_done(self) -> None:
        raise NotImplementedError(
            "No need to call this method in async extension"
        )

    async def on_deinit_done(self) -> None:
        raise NotImplementedError(
            "No need to call this method in async extension"
        )

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
