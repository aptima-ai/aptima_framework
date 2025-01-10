#
# Copyright © 2025 Agora
# This file is part of APTIMA Framework, an open source project.
# Licensed under the Apache License, Version 2.0, with certain conditions.
# Refer to the "LICENSE" file in the root directory for more information.
#
import asyncio

from aptima import AsyncExtension, AsyncTenEnv, Cmd


class DefaultAsyncExtension(AsyncExtension):
    async def on_configure(self, aptima_env: AsyncTenEnv) -> None:
        await asyncio.sleep(0.5)
        await aptima_env.init_property_from_json('{"testKey": "testValue"}')

    async def on_init(self, aptima_env: AsyncTenEnv) -> None:
        await asyncio.sleep(0.5)
        await aptima_env.set_property_bool("bool_field", True)
        await aptima_env.set_property_int("int_field", 1)
        await aptima_env.set_property_float("float_field", 1.0)
        await aptima_env.set_property_string("string_field", "hello")
        await aptima_env.set_property_from_json("json_field", '"testValue2"')

    async def on_start(self, aptima_env: AsyncTenEnv) -> None:
        await asyncio.sleep(0.5)
        aptima_env.log_debug("on_start")

        assert await aptima_env.is_property_exist("unknown_field") is False
        assert await aptima_env.is_property_exist("string_field") is True

        bool_field = await aptima_env.get_property_bool("bool_field")
        assert bool_field is True

        int_field = await aptima_env.get_property_int("int_field")
        assert int_field == 1

        float_field = await aptima_env.get_property_float("float_field")
        assert float_field == 1.0

        string_field = await aptima_env.get_property_string("string_field")
        assert string_field == "hello"

        json_field = await aptima_env.get_property_to_json("json_field")
        assert json_field == '"testValue2"'

        error_occurred = False

        try:
            _ = await aptima_env.get_property_string("unknown_field")
        except Exception:
            error_occurred = True
        finally:
            assert error_occurred is True
            error_occurred = False

        try:
            _ = await aptima_env.get_property_bool("unknown_field")
        except Exception:
            error_occurred = True
        finally:
            assert error_occurred is True
            error_occurred = False

        try:
            _ = await aptima_env.get_property_int("unknown_field")
        except Exception:
            error_occurred = True
        finally:
            assert error_occurred is True
            error_occurred = False

        try:
            _ = await aptima_env.get_property_float("unknown_field")
        except Exception:
            error_occurred = True
        finally:
            assert error_occurred is True
            error_occurred = False

        try:
            _ = await aptima_env.get_property_to_json("unknown_field")
        except Exception:
            error_occurred = True
        finally:
            assert error_occurred is True

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
