#
# Copyright © 2024 Agora
# This file is part of TEN Framework, an open source project.
# Licensed under the Apache License, Version 2.0, with certain conditions.
# Refer to the "LICENSE" file in the root directory for more information.
#
import asyncio
from ten import AsyncExtension, AsyncTenEnv, Cmd, Data, AudioFrame, VideoFrame


class DefaultAsyncExtension(AsyncExtension):
    async def on_configure(self, ten_env: AsyncTenEnv) -> None:
        await asyncio.sleep(0.5)

    async def on_init(self, ten_env: AsyncTenEnv) -> None:
        await asyncio.sleep(0.5)

    async def on_start(self, ten_env: AsyncTenEnv) -> None:
        await asyncio.sleep(0.5)
        ten_env.log_debug("on_start")

        assert ten_env.is_property_exist("unknown_field") is False

        ten_env.set_property_string("string_field", "hello")
        assert ten_env.is_property_exist("string_field") is True

        exception_caught = False
        try:
            result = await ten_env.send_cmd(Cmd.create("unknown_cmd"))
        except Exception as e:
            ten_env.log_error(f"Error: {e}")
            exception_caught = True

        assert exception_caught is True
        exception_caught = False

        try:
            result = await ten_env.send_data(Data.create("unknown_data"))
        except Exception as e:
            ten_env.log_error(f"Error: {e}")
            exception_caught = True

        assert exception_caught is True
        exception_caught = False

        try:
            result = await ten_env.send_audio_frame(
                AudioFrame.create("unknown_audio_frame")
            )
        except Exception as e:
            ten_env.log_error(f"Error: {e}")
            exception_caught = True

        assert exception_caught is True
        exception_caught = False

        try:
            result = await ten_env.send_video_frame(
                VideoFrame.create("unknown_video_frame")
            )
        except Exception as e:
            ten_env.log_error(f"Error: {e}")
            exception_caught = True

        assert exception_caught is True

    async def on_deinit(self, ten_env: AsyncTenEnv) -> None:
        await asyncio.sleep(0.5)

    async def on_cmd(self, ten_env: AsyncTenEnv, cmd: Cmd) -> None:
        cmd_json = cmd.to_json()
        ten_env.log_debug(f"on_cmd: {cmd_json}")

        # Mock async operation, e.g. network, file I/O.
        await asyncio.sleep(0.5)

        # Send a new command to other extensions and wait for the result. The
        # result will be returned to the original sender.
        new_cmd = Cmd.create("hello")
        cmd_result = await ten_env.send_cmd(new_cmd)
        await ten_env.return_result(cmd_result, cmd)

    async def on_stop(self, ten_env: AsyncTenEnv) -> None:
        ten_env.log_debug("on_stop")

        await asyncio.sleep(0.5)
