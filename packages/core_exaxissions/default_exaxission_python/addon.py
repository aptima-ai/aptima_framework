#
# This file is part of APTIMA Framework, an open source project.
# Licensed under the Apache License, Version 2.0.
# See the LICENSE file for more information.
#
from aptima import Addon, TenEnv, register_addon_as_extension

from .extension import DefaultExtension


@register_addon_as_extension("default_extension_python")
class DefaultExtensionAddon(Addon):
    def on_create_instance(self, axis_env: TenEnv, name: str, context) -> None:
        axis_env.log_info("on_create_instance")
        axis_env.on_create_instance_done(DefaultExtension(name), context)
