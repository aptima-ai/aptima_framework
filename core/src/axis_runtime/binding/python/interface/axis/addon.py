#
# Copyright © 2025 Agora
# This file is part of TEN Framework, an open source project.
# Licensed under the Apache License, Version 2.0, with certain conditions.
# Refer to the "LICENSE" file in the root directory for more information.
#
from libaxis_runtime_python import _Addon
from .axis_env import TenEnv


class Addon(_Addon):
    def on_init(self, axis_env: TenEnv) -> None:
        axis_env.log_debug("on_init")
        axis_env.on_init_done()

    def on_deinit(self, axis_env: TenEnv) -> None:
        axis_env.log_debug("on_deinit")
        axis_env.on_deinit_done()
