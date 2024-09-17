#
# Copyright © 2024 Agora
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

    def on_init(self, ten_env: TenEnv) -> None:
        print(f"{self.name} on_init")
        ten_env.on_init_done()

    def on_cmd(self, ten_env: TenEnv, cmd: Cmd) -> None:
        print(f"{self.name} on_cmd")

        cmd_json = cmd.to_json()
        print(f"{self.name} on_cmd json: {cmd_json}")

        if self.name == "default_extension_python_1":
            if cmd.get_name() == "test":
                self.cached_cmd = cmd
                new_cmd = Cmd.create("hello")
                ten_env.send_cmd(new_cmd, None)
            elif cmd.get_name() == "hello2":
                cmd_result = CmdResult.create(StatusCode.OK)
                cmd_result.set_property_string("detail", "nbnb")
                ten_env.return_result(cmd_result, self.cached_cmd)
        elif self.name == "default_extension_python_2":
            print(f"{self.name} create respCmd 1")
            ten_env.return_result(CmdResult.create(StatusCode.OK), cmd)

            print(f"{self.name} create respCmd 2")
            json_str = """{"_ten": {"name": "hello2"}}"""
            ten_env.send_json(json_str, None)
