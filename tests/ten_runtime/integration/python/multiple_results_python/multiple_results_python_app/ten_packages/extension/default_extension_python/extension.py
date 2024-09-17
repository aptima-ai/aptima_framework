#
# This file is part of the TEN Framework project.
# See https://github.com/TEN-framework/ten_framework/LICENSE for license
# information.
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

        self.__counter = 0

    def on_init(self, ten_env: TenEnv) -> None:
        ten_env.log_debug("on_init")
        ten_env.on_init_done()

    def check_hello(self, ten_env: TenEnv, result: CmdResult, receivedCmd: Cmd):
        self.__counter += 1

        if self.__counter == 1:
            assert result.get_is_final() is False
            ten_env.log_info("receive 1 cmd result")
        elif self.__counter == 2:
            assert result.get_is_final() is True
            ten_env.log_info("receive 2 cmd result")

            respCmd = CmdResult.create(StatusCode.OK)
            respCmd.set_property_string("detail", "nbnb")
            ten_env.return_result(respCmd, receivedCmd)

    def on_cmd(self, ten_env: TenEnv, cmd: Cmd) -> None:
        cmd_json = cmd.to_json()
        ten_env.log_debug(f"on_cmd json: {cmd_json}")

        if self.name == "default_extension_python_1":
            new_cmd = Cmd.create("hello")
            ten_env.send_cmd(
                new_cmd, lambda ten, result: self.check_hello(ten, result, cmd)
            )
        elif self.name == "default_extension_python_2":
            ten_env.log_info("create respCmd 1")
            respCmd = CmdResult.create(StatusCode.OK)
            # The following line is the key.
            respCmd.set_is_final(False)
            ten_env.return_result(respCmd, cmd)

            ten_env.log_info("create respCmd 2")
            respCmd = CmdResult.create(StatusCode.OK)
            ten_env.return_result(respCmd, cmd)
