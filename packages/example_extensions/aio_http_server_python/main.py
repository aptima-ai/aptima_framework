#
# This file is part of TEN Framework, an open source project.
# Licensed under the Apache License, Version 2.0.
# See the LICENSE file for more information.
#
import asyncio
import json
from aiohttp import web, web_request, WSMsgType
from ten import (
    Addon,
    AsyncExtension,
    register_addon_as_extension,
    TenEnv,
    Cmd,
    CmdResult,
    StatusCode,
    AsyncTenEnv,
)


class HttpServerExtension(AsyncExtension):
    async def default_handler(self, request: web_request.Request):
        # Parse the json body.
        try:
            data = await request.json()
        except Exception as e:
            self.ten_env.log_error("Error: " + str(e))
            return web.Response(status=400, text="Bad request")

        cmd_result = None

        method = request.method
        url = request.path

        if "_ten" not in data:
            return web.Response(status=404, text="Not found")
        else:
            # If the command is a 'close_app' command, send it to the app.
            if "type" in data["_ten"] and data["_ten"]["type"] == "close_app":
                close_app_cmd_json = (
                    '{"_ten":{"type":"close_app",'
                    '"dest":[{"app":"localhost"}]}}'
                )
                asyncio.create_task(
                    anext(self.ten_env.send_json(close_app_cmd_json))
                )
                return web.Response(status=200, text="OK")
            elif "name" in data["_ten"]:
                # Send the command to the TEN runtime.
                data["method"] = method
                data["url"] = url
                cmd = Cmd.create_from_json(json.dumps(data))

                # Send the command to the TEN runtime and wait for the result.
                if cmd is None:
                    return web.Response(status=400, text="Bad request")

                cmd_result = await anext(self.ten_env.send_cmd(cmd))
            else:
                return web.Response(status=404, text="Not found")

        if cmd_result.get_status_code() == StatusCode.OK:
            try:
                detail = cmd_result.get_property_string("detail")
                return web.Response(text=detail)
            except Exception as e:
                self.ten_env.log_error("Error: " + str(e))
                return web.Response(status=500, text="Internal server error")
        else:
            return web.Response(status=500, text="Internal server error")

    async def default_ws_handler(self, request: web_request.Request):
        ws = web.WebSocketResponse()
        await ws.prepare(request)

        async for msg in ws:
            if msg.type == WSMsgType.TEXT:
                if msg.data == "close":
                    await ws.close()
                else:
                    await ws.send_str("some websocket message payload")
            elif msg.type == WSMsgType.ERROR:
                self.ten_env.log_error(
                    "ws connection closed with exception %s" % ws.exception()
                )

        return ws

    def create_runner(self):
        self.webApp = web.Application()
        self.webApp.add_routes(
            [
                web.post("/", self.default_handler),
                web.get("/ws", self.default_ws_handler),
            ]
        )
        return web.AppRunner(self.webApp)

    async def start_server(self, host, port):
        runner = self.create_runner()
        await runner.setup()
        self.tcpSite = web.TCPSite(runner, host, port)
        await self.tcpSite.start()

    def __init__(self, name: str) -> None:
        super().__init__(name)
        self.name = name

    async def on_init(self, ten_env: AsyncTenEnv) -> None:
        self.ten_env = ten_env

    async def on_start(self, ten_env: AsyncTenEnv) -> None:
        ten_env.log_debug("on_start")

        try:
            self.server_port = ten_env.get_property_int("server_port")
        except Exception as e:
            ten_env.log_error(
                "Could not read 'server_port' from properties." + str(e)
            )
            self.server_port = 8002

        await self.start_server("0.0.0.0", self.server_port)

    async def on_deinit(self, ten_env: AsyncTenEnv) -> None:
        ten_env.log_debug("on_deinit")

    async def on_cmd(self, ten_env: AsyncTenEnv, cmd: Cmd) -> None:
        ten_env.log_debug("on_cmd")

        # Not supported command.
        ten_env.return_result(CmdResult.create(StatusCode.ERROR), cmd)

    async def on_stop(self, ten_env: AsyncTenEnv) -> None:
        ten_env.log_debug("on_stop")


@register_addon_as_extension("aio_http_server_python")
class DefaultExtensionAddon(Addon):
    def on_create_instance(self, ten_env: TenEnv, name: str, context) -> None:
        print("DefaultExtensionAddon on_create_instance")
        ten_env.on_create_instance_done(HttpServerExtension(name), context)
