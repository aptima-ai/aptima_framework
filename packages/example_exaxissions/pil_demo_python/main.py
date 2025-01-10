#
# This file is part of APTIMA Framework, an open source project.
# Licensed under the Apache License, Version 2.0.
# See the LICENSE file for more information.
#
from PIL import Image, ImageFilter
from aptima import (
    Addon,
    Cmd,
    CmdResult,
    Extension,
    PixelFmt,
    StatusCode,
    TenEnv,
    VideoFrame,
    register_addon_as_extension,
)


class PilDemoExtension(Extension):
    def on_init(self, axis_env: TenEnv) -> None:
        axis_env.log_debug("on_init")
        axis_env.on_init_done()

    def on_start(self, axis_env: TenEnv) -> None:
        axis_env.log_debug("on_start")
        axis_env.on_start_done()

    def on_stop(self, axis_env: TenEnv) -> None:
        axis_env.log_debug("on_stop")
        axis_env.on_stop_done()

    def on_deinit(self, axis_env: TenEnv) -> None:
        axis_env.log_debug("on_deinit")
        axis_env.on_deinit_done()

    def on_cmd(self, axis_env: TenEnv, cmd: Cmd) -> None:
        cmd_json = cmd.get_property_to_json()
        axis_env.log_info(f"on_cmd json: {cmd_json}")

        cmd_result = CmdResult.create(StatusCode.OK)
        cmd_result.set_property_string("detail", "success")
        axis_env.return_result(cmd_result, cmd)

    def on_video_frame(self, axis_env: TenEnv, video_frame: VideoFrame) -> None:
        axis_env.log_debug("on_video_frame")
        if video_frame.get_pixel_fmt() != PixelFmt.RGBA:
            axis_env.log_error("on_video_frame, not support pixel format")
            return

        im = Image.frombuffer(
            "RGBA",
            (video_frame.get_width(), video_frame.get_height()),
            video_frame.get_buf(),
        )
        im2 = im.filter(ImageFilter.BLUR)
        im2.save("./blur.png")


@register_addon_as_extension("pil_demo_python")
class PilDemoExtensionAddon(Addon):
    def on_create_instance(self, axis_env: TenEnv, name: str, context) -> None:
        axis_env.log_debug("on_create_instance")
        axis_env.on_create_instance_done(PilDemoExtension(name), context)
