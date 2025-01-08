#
# Copyright © 2025 Agora
# This file is part of TEN Framework, an open source project.
# Licensed under the Apache License, Version 2.0, with certain conditions.
# Refer to the "LICENSE" file in the root directory for more information.
#

# import debugpy
# debugpy.listen(5678)
# debugpy.wait_for_client()

import asyncio
from typing import Optional
from ten import (
    AsyncExtension,
    AsyncTenEnv,
    Cmd,
    StatusCode,
    CmdResult,
)


class DefaultExtension(AsyncExtension):
    async def on_configure(self, axis_env: AsyncTenEnv) -> None:
        axis_env.log_debug("on_init")

        await axis_env.init_property_from_json('{"testKey": "testValue"}')

        await asyncio.sleep(0.5)

    async def on_start(self, axis_env: AsyncTenEnv) -> None:
        axis_env.log_debug("on_start")

        await asyncio.sleep(0.5)

        await axis_env.set_property_from_json("testKey2", '"testValue2"')
        testValue = await axis_env.get_property_to_json("testKey")
        testValue2 = await axis_env.get_property_to_json("testKey2")
        axis_env.log_info(f"testValue: {testValue}, testValue2: {testValue2}")

    async def on_stop(self, axis_env: AsyncTenEnv) -> None:
        axis_env.log_debug("on_stop")

        await asyncio.sleep(0.5)

    async def on_deinit(self, axis_env: AsyncTenEnv) -> None:
        axis_env.log_debug("on_deinit")

        await asyncio.sleep(0.5)

    async def greeting(self, axis_env: AsyncTenEnv) -> Optional[CmdResult]:
        await asyncio.sleep(1)

        new_cmd = Cmd.create("greeting")
        result, err = await axis_env.send_cmd(new_cmd)
        if err is not None:
            axis_env.log_fatal(f"greeting error: {err.err_msg()}")

        return result

    async def on_cmd(self, axis_env: AsyncTenEnv, cmd: Cmd) -> None:
        cmd_json = cmd.get_property_to_json()
        axis_env.log_debug("on_cmd: " + cmd_json)

        new_cmd = Cmd.create("hello")
        new_cmd.set_property_from_json("test", '"testValue2"')
        test_value = new_cmd.get_property_to_json("test")
        axis_env.log_info(f"on_cmd test_value: {test_value}")

        await asyncio.sleep(0.5)

        result, _ = await axis_env.send_cmd(new_cmd)
        assert result is not None

        statusCode = result.get_status_code()
        detail = result.get_property_string("detail")
        axis_env.log_info(
            f"check_hello: status: {str(statusCode)}, detail: {detail}"
        )

        greeting_tasks = [self.greeting(axis_env) for _ in range(100)]

        results = await asyncio.gather(*greeting_tasks)

        for result in results:
            if result is None:
                axis_env.log_fatal("check_hello: result is None")
                assert False

            statusCode = result.get_status_code()
            if statusCode != StatusCode.OK:
                axis_env.log_fatal(f"check_hello: status: {str(statusCode)}")
                assert False

        respCmd = CmdResult.create(StatusCode.OK)
        respCmd.set_property_string("detail", "received response")
        axis_env.log_info("create respCmd")

        await axis_env.return_result(respCmd, cmd)
