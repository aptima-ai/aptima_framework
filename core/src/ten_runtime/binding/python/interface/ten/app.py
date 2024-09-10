#
# This file is part of the TEN Framework project.
# See https://github.com/TEN-framework/ten_framework/LICENSE for license
# information.
#
from libten_runtime_python import _App
from .ten_env import TenEnv


class App(_App):
    def run(self, run_in_background: bool) -> None:
        if run_in_background:
            _App.run(self, 1)
        else:
            _App.run(self, 0)

    def wait(self) -> None:
        _App.wait(self)

    def close(self) -> None:
        _App.close(self)

    def on_init(self, ten_env: TenEnv) -> None:
        ten_env.on_init_done()
