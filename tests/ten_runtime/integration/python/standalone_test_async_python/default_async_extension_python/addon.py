#
# This file is part of APTIMA Framework, an open source project.
# Licensed under the Apache License, Version 2.0.
# See the LICENSE file for more information.
#
from aptima import Addon, TenEnv, register_addon_as_extension

from .extension import DefaultAsyncExtension


@register_addon_as_extension("default_async_extension_python")
class DefaultAsyncExtensionAddon(Addon):
    def on_create_instance(self, ten_env: TenEnv, name: str, context) -> None:
        ten_env.log_info("on_create_instance")
        ten_env.on_create_instance_done(DefaultAsyncExtension(name), context)
