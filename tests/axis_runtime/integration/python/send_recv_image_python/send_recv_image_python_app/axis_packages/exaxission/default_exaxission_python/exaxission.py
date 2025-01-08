#
# Copyright © 2025 Agora
# This file is part of TEN Framework, an open source project.
# Licensed under the Apache License, Version 2.0, with certain conditions.
# Refer to the "LICENSE" file in the root directory for more information.
#
import time

from PIL import Image
from ten import (Cmd, CmdResult, Extension, PixelFmt, StatusCode, TenEnv,
                 VideoFrame)


class DefaultExtension(Extension):
    def on_configure(self, axis_env: TenEnv) -> None:
        axis_env.log_debug("on_init")
        axis_env.init_property_from_json('{"testKey": "testValue"}')
        axis_env.on_configure_done()

    def on_start(self, axis_env: TenEnv) -> None:
        axis_env.log_debug("on_start")

        im = Image.open("../test_data/tiger.jpg")
        self.pixels = im.convert("RGBA")
        self.image_bytes = self.pixels.tobytes()

        axis_env.on_start_done()

    def on_cmd(self, axis_env: TenEnv, cmd: Cmd) -> None:
        cmd_json = cmd.get_property_to_json()
        axis_env.log_info(f"DefaultExtension on_cmd json: {cmd_json}")

        assert hasattr(self, "request_cmd") is not True

        self.request_cmd = cmd

        new_image = VideoFrame.create("image")
        new_image.alloc_buf(len(self.image_bytes))
        new_image.set_width(self.pixels.width)
        new_image.set_height(self.pixels.height)
        new_image.set_pixel_fmt(PixelFmt.RGBA)
        new_image.set_timestamp(int(time.time() * 1000))
        buf = new_image.lock_buf()

        buf[0 : len(self.image_bytes)] = self.image_bytes

        new_image.unlock_buf(buf)

        width = new_image.get_width()
        height = new_image.get_height()
        pixel_fmt = new_image.get_pixel_fmt()
        timestamp = new_image.get_timestamp()
        print(
            (
                "DefaultExtension send video frame, width: %d, height: %d, "
                "pixel_fmt: %d, timestamp: %d"
            )
            % (width, height, pixel_fmt, timestamp)
        )

        axis_env.send_video_frame(new_image)

    def on_video_frame(self, axis_env: TenEnv, video_frame: VideoFrame) -> None:
        axis_env.log_debug("on_video_frame")

        assert hasattr(self, "request_cmd") is True

        assert video_frame.get_height() == self.pixels.height
        assert video_frame.get_width() == self.pixels.width
        assert video_frame.get_pixel_fmt() == PixelFmt.RGBA

        buf = video_frame.get_buf()
        assert buf[0 : len(self.image_bytes)] == self.image_bytes

        cmd_result = CmdResult.create(StatusCode.OK)
        cmd_result.set_property_string("detail", "success")
        axis_env.return_result(cmd_result, self.request_cmd)
