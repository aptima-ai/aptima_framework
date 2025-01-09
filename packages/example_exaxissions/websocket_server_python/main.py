#
# This file is part of TEN Framework, an open source project.
# Licensed under the Apache License, Version 2.0.
# See the LICENSE file for more information.
#
import websockets
from ten import (Addon, AsyncExtension, AsyncTenEnv, Cmd, CmdResult,
                 StatusCode, TenEnv, register_addon_as_extension)


class WebsocketServerExtension(AsyncExtension):
    def __init__(self, name: str) -> None:
        super().__init__(name)
        self.name = name

    async def on_init(self, aptima_env: AsyncTenEnv) -> None:
        self.aptima_env = aptima_env

    async def echo(self, websocket, path):
        async for message in websocket:
            print(f"Received message: {message}")
            # Echo the message back to the client
            await websocket.send(f"Server received: {message}")

    async def on_start(self, aptima_env: AsyncTenEnv) -> None:
        aptima_env.log_debug("on_start")

        try:
            self.server_port = await aptima_env.get_property_int("server_port")
        except Exception as e:
            aptima_env.log_error(
                "Could not read 'server_port' from properties." + str(e)
            )
            self.server_port = 8002

        self.server = websockets.serve(self.echo, "localhost", self.server_port)
        self.aptima_env.log_debug(
            f"Websocket server started on port {self.server_port}"
        )

        await self.server
        print("Websocket server started.")

    async def on_deinit(self, aptima_env: AsyncTenEnv) -> None:
        aptima_env.log_debug("on_deinit")

    async def on_cmd(self, aptima_env: AsyncTenEnv, cmd: Cmd) -> None:
        aptima_env.log_debug("on_cmd")

        # Not supported command.
        await aptima_env.return_result(CmdResult.create(StatusCode.ERROR), cmd)

    async def on_stop(self, aptima_env: AsyncTenEnv) -> None:
        aptima_env.log_debug("on_stop")
        self.server.ws_server.close()


@register_addon_as_extension("websocket_server_python")
class DefaultExtensionAddon(Addon):
    def on_create_instance(self, aptima_env: TenEnv, name: str, context) -> None:
        print("on_create_instance")
        aptima_env.on_create_instance_done(WebsocketServerExtension(name), context)
