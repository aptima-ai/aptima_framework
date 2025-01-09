#
# Copyright © 2025 Agora
# This file is part of TEN Framework, an open source project.
# Licensed under the Apache License, Version 2.0, with certain conditions.
# Refer to the "LICENSE" file in the root directory for more information.
#
import asyncio

from ten import AsyncExtension, AsyncTenEnv, Cmd


class DefaultExtension(AsyncExtension):
    def __init__(self, name: str) -> None:
        super().__init__(name)
        self.name = name

    async def on_configure(self, aptima_env: AsyncTenEnv) -> None:
        await asyncio.sleep(0.5)

    async def on_init(self, aptima_env: AsyncTenEnv) -> None:
        await asyncio.sleep(0.5)

    async def on_start(self, aptima_env: AsyncTenEnv) -> None:
        await asyncio.sleep(0.5)
        aptima_env.log_debug("on_start")

    async def on_deinit(self, aptima_env: AsyncTenEnv) -> None:
        await asyncio.sleep(0.5)
        await asyncio.sleep(1)

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
