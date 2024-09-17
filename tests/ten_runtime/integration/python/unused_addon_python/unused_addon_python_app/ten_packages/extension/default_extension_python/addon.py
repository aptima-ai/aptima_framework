#
# Copyright © 2024 Agora
# This file is part of TEN Framework, an open source project.
# Licensed under the Apache License, Version 2.0, with certain conditions.
# Refer to the "LICENSE" file in the root directory for more information.
#
from ten import Addon, register_addon_as_extension, TenEnv


@register_addon_as_extension("default_extension_python")
class DefaultExtensionAddon(Addon):

    def on_init(self, ten_env: TenEnv) -> None:
        print("DefaultExtensionAddon on_init")
        ten_env.on_init_done()
        return

    def on_create_instance(self, ten_env: TenEnv, name: str, context) -> None:
        print("DefaultExtensionAddon on_create_instance")
        from . import extension

        ten_env.on_create_instance_done(
            extension.DefaultExtension(name), context
        )

    def on_deinit(self, ten_env: TenEnv) -> None:
        print("DefaultExtensionAddon on_deinit")
        ten_env.on_deinit_done()
        return
