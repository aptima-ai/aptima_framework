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

    async def on_init(self, axis_env: AsyncTenEnv) -> None:
        self.axis_env = axis_env

    async def echo(self, websocket, path):
        async for message in websocket:
            print(f"Received message: {message}")
            # Echo the message back to the client
            await websocket.send(f"Server received: {message}")

    async def on_start(self, axis_env: AsyncTenEnv) -> None:
        axis_env.log_debug("on_start")

        try:
            self.server_port = await axis_env.get_property_int("server_port")
        except Exception as e:
            axis_env.log_error(
                "Could not read 'server_port' from properties." + str(e)
            )
            self.server_port = 8002

        self.server = websockets.serve(self.echo, "localhost", self.server_port)
        self.axis_env.log_debug(
            f"Websocket server started on port {self.server_port}"
        )

        await self.server
        print("Websocket server started.")

    async def on_deinit(self, axis_env: AsyncTenEnv) -> None:
        axis_env.log_debug("on_deinit")

    async def on_cmd(self, axis_env: AsyncTenEnv, cmd: Cmd) -> None:
        axis_env.log_debug("on_cmd")

        # Not supported command.
        await axis_env.return_result(CmdResult.create(StatusCode.ERROR), cmd)

    async def on_stop(self, axis_env: AsyncTenEnv) -> None:
        axis_env.log_debug("on_stop")
        self.server.ws_server.close()


@register_addon_as_extension("websocket_server_python")
class DefaultExtensionAddon(Addon):
    def on_create_instance(self, axis_env: TenEnv, name: str, context) -> None:
        print("on_create_instance")
        axis_env.on_create_instance_done(WebsocketServerExtension(name), context)
