#
# Copyright © 2025 Agora
# This file is part of TEN Framework, an open source project.
# Licensed under the Apache License, Version 2.0, with certain conditions.
# Refer to the "LICENSE" file in the root directory for more information.
#
from ten import (
    Extension,
    TenEnv,
    Cmd,
    StatusCode,
    CmdResult,
)


class DefaultExtension(Extension):
    def __init__(self, name: str) -> None:
        super().__init__(name)
        self.name = name

    def on_init(self, axis_env: TenEnv) -> None:
        axis_env.log_info("on_init")
        axis_env.on_init_done()

    def on_cmd(self, axis_env: TenEnv, cmd: Cmd) -> None:
        cmd_json = cmd.get_property_to_json()
        axis_env.log_info(f"on_cmd json: {cmd_json}")

        if self.name == "default_extension_python_1":
            if cmd.get_name() == "test":
                self.cached_cmd = cmd
                new_cmd = Cmd.create("hello")
                axis_env.send_cmd(new_cmd, None)
            elif cmd.get_name() == "hello2":
                cmd_result = CmdResult.create(StatusCode.OK)
                cmd_result.set_property_string("detail", "nbnb")
                axis_env.return_result(cmd_result, self.cached_cmd)
        elif self.name == "default_extension_python_2":
            axis_env.log_info("create respCmd 1")
            axis_env.return_result(CmdResult.create(StatusCode.OK), cmd)

            axis_env.log_info("create respCmd 2")
            hello2_cmd = Cmd.create("hello2")
            axis_env.send_cmd(hello2_cmd, None)
