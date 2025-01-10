#
# Copyright © 2025 Agora
# This file is part of APTIMA Framework, an open source project.
# Licensed under the Apache License, Version 2.0, with certain conditions.
# Refer to the "LICENSE" file in the root directory for more information.
#
import asyncio

from aptima import (
    AsyncExtension,
    AsyncTenEnv,
    AudioFrame,
    Cmd,
    Data,
    VideoFrame,
)


class DefaultAsyncExtension(AsyncExtension):
    async def on_configure(self, ten_env: AsyncTenEnv) -> None:
        await asyncio.sleep(0.5)

    async def on_init(self, ten_env: AsyncTenEnv) -> None:
        await asyncio.sleep(0.5)

    async def on_start(self, ten_env: AsyncTenEnv) -> None:
        await asyncio.sleep(0.5)
        ten_env.log_debug("on_start")

        assert await ten_env.is_property_exist("unknown_field") is False

        await ten_env.set_property_string("string_field", "hello")
        assert await ten_env.is_property_exist("string_field") is True

        result, err = await ten_env.send_cmd(Cmd.create("unknown_cmd"))
        assert err is not None

        ten_env.log_error(f"Error: {err.err_msg()}")

        err = await ten_env.send_data(Data.create("unknown_data"))
        assert err is not None

        ten_env.log_error(f"Error: {err.err_msg()}")

        err = await ten_env.send_audio_frame(
            AudioFrame.create("unknown_audio_frame")
        )
        assert err is not None

        ten_env.log_error(f"Error: {err.err_msg()}")

        err = await ten_env.send_video_frame(
            VideoFrame.create("unknown_video_frame")
        )
        assert err is not None

        ten_env.log_error(f"Error: {err.err_msg()}")

    async def on_deinit(self, ten_env: AsyncTenEnv) -> None:
        await asyncio.sleep(0.5)

    async def on_cmd(self, ten_env: AsyncTenEnv, cmd: Cmd) -> None:
        cmd_json = cmd.get_property_to_json()
        ten_env.log_debug(f"on_cmd: {cmd_json}")

        # Mock async operation, e.g. network, file I/O.
        await asyncio.sleep(0.5)

        # Send a new command to other extensions and wait for the result. The
        # result will be returned to the original sender.
        new_cmd = Cmd.create("hello")
        cmd_result, _ = await ten_env.send_cmd(new_cmd)
        assert cmd_result is not None

        await ten_env.return_result(cmd_result, cmd)

    async def on_stop(self, ten_env: AsyncTenEnv) -> None:
        ten_env.log_debug("on_stop")

        await asyncio.sleep(0.5)
