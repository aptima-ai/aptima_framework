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
