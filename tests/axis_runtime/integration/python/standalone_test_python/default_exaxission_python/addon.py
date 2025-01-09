#
# Copyright © 2025 Agora
# This file is part of TEN Framework, an open source project.
# Licensed under the Apache License, Version 2.0, with certain conditions.
# Refer to the "LICENSE" file in the root directory for more information.
#
from ten import Addon, TenEnv, register_addon_as_extension

from .extension import DefaultExtension


@register_addon_as_extension("default_extension_python")
class DefaultExtensionAddon(Addon):
    def on_create_instance(self, aptima_env: TenEnv, name: str, context) -> None:
        aptima_env.log_info("on_create_instance")
        aptima_env.on_create_instance_done(DefaultExtension(name), context)
