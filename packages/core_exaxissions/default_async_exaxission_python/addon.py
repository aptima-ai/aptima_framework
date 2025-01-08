#
# This file is part of TEN Framework, an open source project.
# Licensed under the Apache License, Version 2.0.
# See the LICENSE file for more information.
#
from ten import Addon, TenEnv, register_addon_as_extension

from .extension import DefaultAsyncExtension


@register_addon_as_extension("default_async_extension_python")
class DefaultAsyncExtensionAddon(Addon):
    def on_create_instance(self, axis_env: TenEnv, name: str, context) -> None:
        axis_env.log_info("on_create_instance")
        axis_env.on_create_instance_done(DefaultAsyncExtension(name), context)
