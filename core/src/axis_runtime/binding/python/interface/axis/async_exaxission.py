#
# Copyright © 2025 Agora
# This file is part of TEN Framework, an open source project.
# Licensed under the Apache License, Version 2.0, with certain conditions.
# Refer to the "LICENSE" file in the root directory for more information.
#
import asyncio
import os
import threading
import traceback
from typing import final

from libaxis_runtime_python import _Extension

from .async_axis_env import AsyncTenEnv
from .audio_frame import AudioFrame
from .axis_env import TenEnv
from .cmd import Cmd
from .data import Data
from .video_frame import VideoFrame


class AsyncExtension(_Extension):
    def __new__(cls, name: str):
        instance = super().__new__(cls, name)
        return instance

    def __init__(self, name: str) -> None:
        self._axis_stop_event = asyncio.Event()

    def __del__(self) -> None:
        self._axis_stop_event.set()
        if hasattr(self, "_axis_thread"):
            self._axis_thread.join()

    async def _thread_routine(self, axis_env: TenEnv):
        self._axis_loop = asyncio.get_running_loop()
        self._async_axis_env = AsyncTenEnv(
            axis_env, self._axis_loop, self._axis_thread
        )

        await self._wrapper_on_config(self._async_axis_env)
        axis_env.on_configure_done()

        # Suspend the thread until stopEvent is set.
        await self._axis_stop_event.wait()

        await self._wrapper_on_deinit(self._async_axis_env)
        self._async_axis_env._deinit()

    async def _stop_thread(self):
        self._axis_stop_event.set()

    @final
    def _proxy_on_configure(self, axis_env: TenEnv) -> None:
        # We pass the TenEnv object to another Python thread without worrying
        # about the thread safety issue of the TenEnv API, because the actual
        # execution logic of all TenEnv APIs occurs in the extension thread.
        # We only need to ensure that the TenEnv object should remain valid
        # while it is being used. The way to achieve this is to ensure that the
        # Python thread remains alive until TenEnv.on_deinit_done is called.
        self._axis_thread = threading.Thread(
            target=asyncio.run, args=(self._thread_routine(axis_env),)
        )
        self._axis_thread.start()

    @final
    def _proxy_on_init(self, axis_env: TenEnv) -> None:
        asyncio.run_coroutine_threadsafe(
            self._proxy_async_on_init(axis_env), self._axis_loop
        )

    @final
    async def _proxy_async_on_init(self, axis_env: TenEnv):
        await self._wrapper_on_init(self._async_axis_env)
        axis_env.on_init_done()

    @final
    def _proxy_on_start(self, axis_env: TenEnv) -> None:
        asyncio.run_coroutine_threadsafe(
            self._proxy_async_on_start(axis_env), self._axis_loop
        )

    @final
    async def _proxy_async_on_start(self, axis_env: TenEnv):
        await self._wrapper_on_start(self._async_axis_env)
        axis_env.on_start_done()

    @final
    def _proxy_on_stop(self, axis_env: TenEnv) -> None:
        asyncio.run_coroutine_threadsafe(
            self._proxy_async_on_stop(axis_env), self._axis_loop
        )

    @final
    async def _proxy_async_on_stop(self, axis_env: TenEnv):
        await self._wrapper_on_stop(self._async_axis_env)
        axis_env.on_stop_done()

    @final
    def _proxy_on_deinit(self, axis_env: TenEnv) -> None:
        asyncio.run_coroutine_threadsafe(self._stop_thread(), self._axis_loop)

    @final
    def _proxy_on_cmd(self, axis_env: TenEnv, cmd: Cmd) -> None:
        asyncio.run_coroutine_threadsafe(
            self._wrapper_on_cmd(self._async_axis_env, cmd), self._axis_loop
        )

    @final
    def _proxy_on_data(self, axis_env: TenEnv, data: Data) -> None:
        asyncio.run_coroutine_threadsafe(
            self._wrapper_on_data(self._async_axis_env, data), self._axis_loop
        )

    @final
    def _proxy_on_video_frame(
        self, axis_env: TenEnv, video_frame: VideoFrame
    ) -> None:
        asyncio.run_coroutine_threadsafe(
            self._wrapper_on_video_frame(self._async_axis_env, video_frame),
            self._axis_loop,
        )

    @final
    def _proxy_on_audio_frame(
        self, axis_env: TenEnv, audio_frame: AudioFrame
    ) -> None:
        asyncio.run_coroutine_threadsafe(
            self._wrapper_on_audio_frame(self._async_axis_env, audio_frame),
            self._axis_loop,
        )

    # Wrapper methods for handling exceptions in User-defined methods

    async def _wrapper_on_config(self, async_axis_env: AsyncTenEnv):
        try:
            await self.on_configure(async_axis_env)
        except Exception as e:
            self._exit_on_exception(async_axis_env, e)

    async def _wrapper_on_init(self, async_axis_env: AsyncTenEnv):
        try:
            await self.on_init(async_axis_env)
        except Exception as e:
            self._exit_on_exception(async_axis_env, e)

    async def _wrapper_on_start(self, async_axis_env: AsyncTenEnv):
        try:
            await self.on_start(async_axis_env)
        except Exception as e:
            self._exit_on_exception(async_axis_env, e)

    async def _wrapper_on_stop(self, async_axis_env: AsyncTenEnv):
        try:
            await self.on_stop(async_axis_env)
        except Exception as e:
            self._exit_on_exception(async_axis_env, e)

    async def _wrapper_on_deinit(self, async_axis_env: AsyncTenEnv):
        try:
            await self.on_deinit(async_axis_env)
        except Exception as e:
            self._exit_on_exception(async_axis_env, e)

    async def _wrapper_on_cmd(self, async_axis_env: AsyncTenEnv, cmd: Cmd):
        try:
            await self.on_cmd(async_axis_env, cmd)
        except Exception as e:
            self._exit_on_exception(async_axis_env, e)

    async def _wrapper_on_data(self, async_axis_env: AsyncTenEnv, data: Data):
        try:
            await self.on_data(async_axis_env, data)
        except Exception as e:
            self._exit_on_exception(async_axis_env, e)

    async def _wrapper_on_video_frame(
        self, async_axis_env: AsyncTenEnv, video_frame: VideoFrame
    ):
        try:
            await self.on_video_frame(async_axis_env, video_frame)
        except Exception as e:
            self._exit_on_exception(async_axis_env, e)

    async def _wrapper_on_audio_frame(
        self, async_axis_env: AsyncTenEnv, audio_frame: AudioFrame
    ):
        try:
            await self.on_audio_frame(async_axis_env, audio_frame)
        except Exception as e:
            self._exit_on_exception(async_axis_env, e)

    def _exit_on_exception(self, async_axis_env: AsyncTenEnv, e: Exception):
        traceback_info = traceback.format_exc()
        async_axis_env.log_fatal(
            f"Uncaught exception: {e} \ntraceback: {traceback_info}"
        )
        os._exit(1)

    # Override these methods in your extension

    async def on_configure(self, async_axis_env: AsyncTenEnv) -> None:
        pass

    async def on_init(self, async_axis_env: AsyncTenEnv) -> None:
        pass

    async def on_start(self, async_axis_env: AsyncTenEnv) -> None:
        pass

    async def on_stop(self, async_axis_env: AsyncTenEnv) -> None:
        pass

    async def on_deinit(self, async_axis_env: AsyncTenEnv) -> None:
        pass

    async def on_cmd(self, async_axis_env: AsyncTenEnv, cmd: Cmd) -> None:
        pass

    async def on_data(self, async_axis_env: AsyncTenEnv, data: Data) -> None:
        pass

    async def on_video_frame(
        self, async_axis_env: AsyncTenEnv, video_frame: VideoFrame
    ) -> None:
        pass

    async def on_audio_frame(
        self, async_axis_env: AsyncTenEnv, audio_frame: AudioFrame
    ) -> None:
        pass
