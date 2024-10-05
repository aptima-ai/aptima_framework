#
# Copyright © 2024 Agora
# This file is part of TEN Framework, an open source project.
# Licensed under the Apache License, Version 2.0, with certain conditions.
# Refer to the "LICENSE" file in the root directory for more information.
#
from ten import Extension, TenEnv, Cmd, StatusCode, CmdResult


class DefaultExtension(Extension):
    def __init__(self, name: str) -> None:
        super().__init__(name)
        self.name = name

    def on_cmd(self, ten_env: TenEnv, cmd: Cmd) -> None:
        cmd_json = cmd.to_json()
        ten_env.log_info("DefaultExtension on_cmd json: " + cmd_json)

        if cmd.get_name() == "hello_world":
            cmd_result = CmdResult.create(StatusCode.OK)
            ten_env.return_result(cmd_result, cmd)
