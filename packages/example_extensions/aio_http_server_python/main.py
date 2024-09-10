#
# This file is part of the TEN Framework project.
# Licensed under the Apache License, Version 2.0.
# See the LICENSE file for more information.
#
import json
import threading
from aiohttp import web, web_request, WSMsgType
import asyncio
from ten import (
    Addon,
    Extension,
    register_addon_as_extension,
    TenEnv,
    Cmd,
    CmdResult,
    StatusCode,
)


class HttpServerExtension(Extension):
    async def default_handler(self, request: web_request.Request):
        # parse the json body
        try:
            data = await request.json()
        except Exception as e:
            print("Error: " + str(e))
            return web.Response(status=400, text="Bad request")

        cmd_result = None

        method = request.method
        url = request.path

        if "_ten" not in data:
            return web.Response(status=404, text="Not found")
        else:
            # If the command is a 'close_app' command, send it to the app.
            if "type" in data["_ten"] and data["_ten"]["type"] == "close_app":
                close_app_cmd_json = """{"_ten":{"type":"close_app","dest":[{"app":"localhost"}]}}"""
                self.tenEnv.send_json(close_app_cmd_json, None)
                return web.Response(status=200, text="OK")
            elif "name" in data["_ten"]:
                # send the command to the TEN runtime
                data["method"] = method
                data["url"] = url
                cmd = Cmd.create_from_json(json.dumps(data))
                # send 'test' command to the TEN runtime and wait for the result
                cmd_result = await self.send_cmd_async(self.tenEnv, cmd)
            else:
                return web.Response(status=404, text="Not found")

        if cmd_result.get_status_code() == StatusCode.OK:
            try:
                detail = cmd_result.get_property_string("detail")
                return web.Response(text=detail)
            except Exception as e:
                print("Error: " + str(e))
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
                print("ws connection closed with exception %s" % ws.exception())

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

    async def __thread_routine(self, ten_env: TenEnv):
        print("HttpServerExtension __thread_routine start")

        self.loop = asyncio.get_running_loop()

        await self.start_server("0.0.0.0", self.server_port)

        ten_env.on_start_done()

        # Suspend the thread until stopEvent is set
        await self.stopEvent.wait()

        await self.webApp.shutdown()
        await self.webApp.cleanup()

    async def stop_thread(self):
        self.stopEvent.set()

    async def send_cmd_async(self, ten_env: TenEnv, cmd: Cmd) -> CmdResult:
        print("HttpServerExtension send_cmd_async")
        q = asyncio.Queue(1)
        ten_env.send_cmd(
            cmd,
            lambda ten, result: asyncio.run_coroutine_threadsafe(
                q.put(result), self.loop
            ),  # type: ignore
        )
        return await q.get()

    def __init__(self, name: str) -> None:
        super().__init__(name)
        self.name = name
        self.stopEvent = asyncio.Event()

    def on_init(self, ten_env: TenEnv) -> None:
        ten_env.on_init_done()

    def on_start(self, ten_env: TenEnv) -> None:
        print("HttpServerExtension on_start")

        try:
            self.server_port = ten_env.get_property_int("server_port")
        except Exception as e:
            print("Could not read 'server_port' from properties." + str(e))
            self.server_port = 8002

        self.tenEnv = ten_env

        self.thread = threading.Thread(
            target=asyncio.run, args=(self.__thread_routine(ten_env),)
        )

        # Then 'on_start_done' will be called in the thread
        self.thread.start()

    def on_deinit(self, ten_env: TenEnv) -> None:
        print("HttpServerExtension on_deinit")
        ten_env.on_deinit_done()

    def on_cmd(self, ten_env: TenEnv, cmd: Cmd) -> None:
        print("HttpServerExtension on_cmd")

        # Not supported command
        ten_env.return_result(CmdResult.create(StatusCode.ERROR), cmd)

    def on_stop(self, ten_env: TenEnv) -> None:
        print("HttpServerExtension on_stop")

        if self.thread.is_alive():
            asyncio.run_coroutine_threadsafe(self.stop_thread(), self.loop)
            self.thread.join()

        ten_env.on_stop_done()


@register_addon_as_extension("aio_http_server_python")
class DefaultExtensionAddon(Addon):
    def on_create_instance(self, ten_env: TenEnv, name: str, context) -> None:
        print("DefaultExtensionAddon on_create_instance")
        ten_env.on_create_instance_done(HttpServerExtension(name), context)
