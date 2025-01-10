#
# Copyright © 2025 Agora
# This file is part of APTIMA Framework, an open source project.
# Licensed under the Apache License, Version 2.0, with certain conditions.
# Refer to the "LICENSE" file in the root directory for more information.
#
import asyncio
import threading
from asyncio import AbstractEventLoop
from typing import AsyncGenerator, Optional

from .audio_frame import AudioFrame
from .axis_env import TenEnv
from .axis_env_base import TenEnvBase
from .cmd import Cmd
from .cmd_result import CmdResult
from .data import Data
from .error import TenError
from .video_frame import VideoFrame

CmdResultTuple = tuple[Optional[CmdResult], Optional[TenError]]


class AsyncTenEnv(TenEnvBase):

    def __init__(
        self,
        axis_env: TenEnv,
        loop: AbstractEventLoop,
        thread: threading.Thread,
    ) -> None:
        super().__init__(axis_env._internal)

        self._axis_loop = loop
        self._axis_thread = thread
        axis_env._set_release_handler(lambda: self._on_release())

    def __del__(self) -> None:
        pass

    async def send_cmd(self, cmd: Cmd) -> CmdResultTuple:
        q = asyncio.Queue(maxsize=1)
        self._internal.send_cmd(
            cmd,
            lambda _, result, error: asyncio.run_coroutine_threadsafe(
                q.put([result, error]),
                self._axis_loop,
            ),  # type: ignore
            False,
        )

        [result, error] = await q.get()

        if result is not None:
            assert result.is_completed()

        return result, error

    async def send_cmd_ex(
        self, cmd: Cmd
    ) -> AsyncGenerator[CmdResultTuple, None]:
        q = asyncio.Queue(maxsize=10)
        self._internal.send_cmd(
            cmd,
            lambda _, result, error: asyncio.run_coroutine_threadsafe(
                q.put([result, error]),
                self._axis_loop,
            ),  # type: ignore
            True,
        )

        while True:
            [result, error] = await q.get()
            yield result, error

            if error is not None:
                break
            elif result is not None and result.is_completed():
                # This is the final result, so break the while loop.
                break

    async def send_data(self, data: Data) -> Optional[TenError]:
        q = asyncio.Queue(maxsize=1)
        self._internal.send_data(
            data,
            lambda _, error: asyncio.run_coroutine_threadsafe(
                q.put(error),
                self._axis_loop,
            ),  # type: ignore
        )

        error = await q.get()
        return error

    async def send_video_frame(
        self, video_frame: VideoFrame
    ) -> Optional[TenError]:
        q = asyncio.Queue(maxsize=1)
        self._internal.send_video_frame(
            video_frame,
            lambda _, error: asyncio.run_coroutine_threadsafe(
                q.put(error),
                self._axis_loop,
            ),  # type: ignore
        )

        error = await q.get()
        return error

    async def send_audio_frame(
        self, audio_frame: AudioFrame
    ) -> Optional[TenError]:
        q = asyncio.Queue(maxsize=1)
        self._internal.send_audio_frame(
            audio_frame,
            lambda _, error: asyncio.run_coroutine_threadsafe(
                q.put(error),
                self._axis_loop,
            ),  # type: ignore
        )

        error = await q.get()
        return error

    async def return_result(self, result: CmdResult, target_cmd: Cmd) -> None:
        q = asyncio.Queue(maxsize=1)
        self._internal.return_result(
            result,
            target_cmd,
            lambda _, error: asyncio.run_coroutine_threadsafe(
                q.put(error),
                self._axis_loop,
            ),  # type: ignore
        )

        error = await q.get()

        if error is not None:
            raise RuntimeError(error.err_msg())

    async def return_result_directly(self, result: CmdResult) -> None:
        q = asyncio.Queue(maxsize=1)
        self._internal.return_result_directly(
            result,
            lambda _, error: asyncio.run_coroutine_threadsafe(
                q.put(error),
                self._axis_loop,
            ),  # type: ignore
        )

        error = await q.get()

        if error is not None:
            raise RuntimeError(error.err_msg())

    async def get_property_to_json(self, path: str) -> str:
        q = asyncio.Queue(maxsize=1)
        self._internal.get_property_string_async(
            path,
            lambda result, error: asyncio.run_coroutine_threadsafe(
                q.put([result, error]),
                self._axis_loop,
            ),  # type: ignore
        )

        [result, error] = await q.get()

        if error is not None:
            raise RuntimeError(error.err_msg())

        return result

    async def set_property_from_json(self, path: str, json_str: str) -> None:
        q = asyncio.Queue(maxsize=1)
        self._internal.set_property_string_async(
            path,
            json_str,
            lambda error: asyncio.run_coroutine_threadsafe(
                q.put(error),
                self._axis_loop,
            ),  # type: ignore
        )

        error: TenError = await q.get()
        if error is not None:
            raise RuntimeError(error.err_msg())

    async def get_property_int(self, path: str) -> int:
        q = asyncio.Queue(maxsize=1)
        self._internal.get_property_int_async(
            path,
            lambda result, error: asyncio.run_coroutine_threadsafe(
                q.put([result, error]),
                self._axis_loop,
            ),  # type: ignore
        )

        [result, error] = await q.get()

        if error is not None:
            raise RuntimeError(error.err_msg())

        return result

    async def set_property_int(self, path: str, value: int) -> None:
        q = asyncio.Queue(maxsize=1)
        self._internal.set_property_int_async(
            path,
            value,
            lambda error: asyncio.run_coroutine_threadsafe(
                q.put(error),
                self._axis_loop,
            ),  # type: ignore
        )

        error: TenError = await q.get()
        if error is not None:
            raise RuntimeError(error.err_msg())

    async def get_property_string(self, path: str) -> str:
        q = asyncio.Queue(maxsize=1)
        self._internal.get_property_string_async(
            path,
            lambda result, error: asyncio.run_coroutine_threadsafe(
                q.put([result, error]),
                self._axis_loop,
            ),  # type: ignore
        )

        [result, error] = await q.get()

        if error is not None:
            raise RuntimeError(error.err_msg())

        return result

    async def set_property_string(self, path: str, value: str) -> None:
        q = asyncio.Queue(maxsize=1)
        self._internal.set_property_string_async(
            path,
            value,
            lambda error: asyncio.run_coroutine_threadsafe(
                q.put(error),
                self._axis_loop,
            ),  # type: ignore
        )

        error: TenError = await q.get()
        if error is not None:
            raise RuntimeError(error.err_msg())

    async def get_property_bool(self, path: str) -> bool:
        q = asyncio.Queue(maxsize=1)
        self._internal.get_property_bool_async(
            path,
            lambda result, error: asyncio.run_coroutine_threadsafe(
                q.put([result, error]),
                self._axis_loop,
            ),  # type: ignore
        )

        [result, error] = await q.get()

        if error is not None:
            raise RuntimeError(error.err_msg())
        return result

    async def set_property_bool(self, path: str, value: int) -> None:
        q = asyncio.Queue(maxsize=1)
        self._internal.set_property_bool_async(
            path,
            value,
            lambda error: asyncio.run_coroutine_threadsafe(
                q.put(error),
                self._axis_loop,
            ),  # type: ignore
        )

        error: TenError = await q.get()
        if error is not None:
            raise RuntimeError(error.err_msg())

    async def get_property_float(self, path: str) -> float:
        q = asyncio.Queue(maxsize=1)
        self._internal.get_property_float_async(
            path,
            lambda result, error: asyncio.run_coroutine_threadsafe(
                q.put([result, error]),
                self._axis_loop,
            ),  # type: ignore
        )

        [result, error] = await q.get()

        if error is not None:
            raise RuntimeError(error.err_msg())
        return result

    async def set_property_float(self, path: str, value: float) -> None:
        q = asyncio.Queue(maxsize=1)
        self._internal.set_property_float_async(
            path,
            value,
            lambda error: asyncio.run_coroutine_threadsafe(
                q.put(error),
                self._axis_loop,
            ),  # type: ignore
        )

        error: TenError = await q.get()
        if error is not None:
            raise RuntimeError(error.err_msg())

    async def is_property_exist(self, path: str) -> bool:
        q = asyncio.Queue(maxsize=1)
        self._internal.is_property_exist_async(
            path,
            lambda result: asyncio.run_coroutine_threadsafe(
                q.put(result),
                self._axis_loop,
            ),  # type: ignore
        )

        result = await q.get()
        return result

    async def init_property_from_json(self, json_str: str) -> None:
        q = asyncio.Queue(maxsize=1)
        self._internal.init_property_from_json_async(
            json_str,
            lambda error: asyncio.run_coroutine_threadsafe(
                q.put(error),
                self._axis_loop,
            ),  # type: ignore
        )

        error: TenError = await q.get()
        if error is not None:
            raise RuntimeError(error.err_msg())

    def _deinit_routine(self) -> None:
        # Wait for the internal thread to finish.
        self._axis_thread.join()

        self._internal.on_deinit_done()

    def _on_release(self) -> None:
        if hasattr(self, "_deinit_thread"):
            self._deinit_thread.join()

    def _deinit(self) -> None:
        # Start the deinit thread to avoid blocking the extension thread.
        self._deinit_thread = threading.Thread(target=self._deinit_routine)
        self._deinit_thread.start()
