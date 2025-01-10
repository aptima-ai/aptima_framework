#
# This file is part of APTIMA Framework, an open source project.
# Licensed under the Apache License, Version 2.0.
# See the LICENSE file for more information.
#
from aptima import Addon, TenEnv, register_addon_as_extension

from .extension import SimpleEchoExtension


@register_addon_as_extension("simple_echo_python")
class SimpleEchoExtensionAddon(Addon):
    def on_create_instance(
        self, aptima_env: TenEnv, name: str, context
    ) -> None:
        aptima_env.on_create_instance_done(SimpleEchoExtension(name), context)
