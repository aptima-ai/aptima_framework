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
    async def on_configure(self, aptima_env: AsyncTenEnv) -> None:
        await asyncio.sleep(0.5)

    async def on_init(self, aptima_env: AsyncTenEnv) -> None:
        await asyncio.sleep(0.5)

    async def on_start(self, aptima_env: AsyncTenEnv) -> None:
        await asyncio.sleep(0.5)
        aptima_env.log_debug("on_start")

        assert await aptima_env.is_property_exist("unknown_field") is False

        await aptima_env.set_property_string("string_field", "hello")
        assert await aptima_env.is_property_exist("string_field") is True

        result, err = await aptima_env.send_cmd(Cmd.create("unknown_cmd"))
        assert err is not None

        aptima_env.log_error(f"Error: {err.err_msg()}")

        err = await aptima_env.send_data(Data.create("unknown_data"))
        assert err is not None

        aptima_env.log_error(f"Error: {err.err_msg()}")

        err = await aptima_env.send_audio_frame(
            AudioFrame.create("unknown_audio_frame")
        )
        assert err is not None

        aptima_env.log_error(f"Error: {err.err_msg()}")

        err = await aptima_env.send_video_frame(
            VideoFrame.create("unknown_video_frame")
        )
        assert err is not None

        aptima_env.log_error(f"Error: {err.err_msg()}")

    async def on_deinit(self, aptima_env: AsyncTenEnv) -> None:
        await asyncio.sleep(0.5)

    async def on_cmd(self, aptima_env: AsyncTenEnv, cmd: Cmd) -> None:
        cmd_json = cmd.get_property_to_json()
        aptima_env.log_debug(f"on_cmd: {cmd_json}")

        # Mock async operation, e.g. network, file I/O.
        await asyncio.sleep(0.5)

        # Send a new command to other extensions and wait for the result. The
        # result will be returned to the original sender.
        new_cmd = Cmd.create("hello")
        cmd_result, _ = await aptima_env.send_cmd(new_cmd)
        assert cmd_result is not None

        await aptima_env.return_result(cmd_result, cmd)

    async def on_stop(self, aptima_env: AsyncTenEnv) -> None:
        aptima_env.log_debug("on_stop")

        await asyncio.sleep(0.5)
