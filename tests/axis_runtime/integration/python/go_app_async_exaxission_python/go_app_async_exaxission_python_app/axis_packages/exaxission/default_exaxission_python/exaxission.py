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

from ten import AsyncExtension, AsyncTenEnv, Cmd, CmdResult, StatusCode


class DefaultExtension(AsyncExtension):
    async def on_configure(self, aptima_env: AsyncTenEnv) -> None:
        aptima_env.log_debug("on_init")

        await aptima_env.init_property_from_json('{"testKey": "testValue"}')

        await asyncio.sleep(0.5)

    async def on_start(self, aptima_env: AsyncTenEnv) -> None:
        aptima_env.log_debug("on_start")

        await asyncio.sleep(0.5)

        await aptima_env.set_property_from_json("testKey2", '"testValue2"')
        testValue = await aptima_env.get_property_to_json("testKey")
        testValue2 = await aptima_env.get_property_to_json("testKey2")
        aptima_env.log_info(f"testValue: {testValue}, testValue2: {testValue2}")

    async def on_stop(self, aptima_env: AsyncTenEnv) -> None:
        aptima_env.log_debug("on_stop")

        await asyncio.sleep(0.5)

    async def on_deinit(self, aptima_env: AsyncTenEnv) -> None:
        aptima_env.log_debug("on_deinit")

        await asyncio.sleep(0.5)

    async def greeting(self, aptima_env: AsyncTenEnv) -> Optional[CmdResult]:
        await asyncio.sleep(1)

        new_cmd = Cmd.create("greeting")
        result, err = await aptima_env.send_cmd(new_cmd)
        if err is not None:
            aptima_env.log_fatal(f"greeting error: {err.err_msg()}")

        return result

    async def on_cmd(self, aptima_env: AsyncTenEnv, cmd: Cmd) -> None:
        cmd_json = cmd.get_property_to_json()
        aptima_env.log_debug("on_cmd: " + cmd_json)

        new_cmd = Cmd.create("hello")
        new_cmd.set_property_from_json("test", '"testValue2"')
        test_value = new_cmd.get_property_to_json("test")
        aptima_env.log_info(f"on_cmd test_value: {test_value}")

        await asyncio.sleep(0.5)

        result, _ = await aptima_env.send_cmd(new_cmd)
        assert result is not None

        statusCode = result.get_status_code()
        detail = result.get_property_string("detail")
        aptima_env.log_info(
            f"check_hello: status: {str(statusCode)}, detail: {detail}"
        )

        greeting_tasks = [self.greeting(aptima_env) for _ in range(100)]

        results = await asyncio.gather(*greeting_tasks)

        for result in results:
            if result is None:
                aptima_env.log_fatal("check_hello: result is None")
                assert False

            statusCode = result.get_status_code()
            if statusCode != StatusCode.OK:
                aptima_env.log_fatal(f"check_hello: status: {str(statusCode)}")
                assert False

        respCmd = CmdResult.create(StatusCode.OK)
        respCmd.set_property_string("detail", "received response")
        aptima_env.log_info("create respCmd")

        await aptima_env.return_result(respCmd, cmd)
