#
# Copyright © 2025 Agora
# This file is part of APTIMA Framework, an open source project.
# Licensed under the Apache License, Version 2.0, with certain conditions.
# Refer to the "LICENSE" file in the root directory for more information.
#
from typing import Optional

from aptima import Cmd, CmdResult, Extension, StatusCode, TenEnv, TenError


class DefaultExtension(Extension):
    def __init__(self, name: str) -> None:
        super().__init__(name)
        self.name = name

        self.__counter = 0

    def on_init(self, aptima_env: TenEnv) -> None:
        aptima_env.log_debug("on_init")
        aptima_env.on_init_done()

    def check_hello(
        self,
        aptima_env: TenEnv,
        result: Optional[CmdResult],
        error: Optional[TenError],
        receivedCmd: Cmd,
    ):
        if error is not None:
            assert False, error

        assert result is not None

        self.__counter += 1

        if self.__counter == 1:
            assert result.is_completed() is True
            aptima_env.log_info("receive 1 cmd result")

            respCmd = CmdResult.create(StatusCode.OK)
            respCmd.set_property_string("detail", "nbnb")
            aptima_env.return_result(respCmd, receivedCmd)
        else:
            assert False

    def on_cmd(self, aptima_env: TenEnv, cmd: Cmd) -> None:
        cmd_json = cmd.get_property_to_json()
        aptima_env.log_debug(f"on_cmd json: {cmd_json}")

        if self.name == "default_extension_python_1":
            new_cmd = Cmd.create("hello")
            aptima_env.send_cmd(
                new_cmd,
                lambda aptima_env, result, error: self.check_hello(
                    aptima_env, result, error, cmd
                ),
            )
        elif self.name == "default_extension_python_2":
            aptima_env.log_info("create respCmd")
            respCmd = CmdResult.create(StatusCode.OK)
            aptima_env.return_result(respCmd, cmd)
        elif self.name == "default_extension_python_3":
            aptima_env.log_info("create respCmd")
            respCmd = CmdResult.create(StatusCode.OK)
            aptima_env.return_result(respCmd, cmd)
