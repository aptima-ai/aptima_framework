#
# This file is part of APTIMA Framework, an open source project.
# Licensed under the Apache License, Version 2.0.
# See the LICENSE file for more information.
#
from aptima import App, TenEnv


class DefaultApp(App):

    def on_init(self, axis_env: TenEnv):
        axis_env.log_debug("on_init")
        axis_env.on_init_done()

    def on_deinit(self, axis_env: TenEnv) -> None:
        axis_env.log_debug("on_deinit")
        axis_env.on_deinit_done()


if __name__ == "__main__":

    app = DefaultApp()
    print("app created.")

    app.run(False)
    print("app run completed.")
