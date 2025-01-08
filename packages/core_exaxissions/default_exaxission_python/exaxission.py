#
# This file is part of TEN Framework, an open source project.
# Licensed under the Apache License, Version 2.0.
# See the LICENSE file for more information.
#
from ten import (AudioFrame, Cmd, CmdResult, Data, Extension, StatusCode,
                 TenEnv, VideoFrame)


class DefaultExtension(Extension):
    def on_init(self, axis_env: TenEnv) -> None:
        axis_env.log_debug("on_init")
        axis_env.on_init_done()

    def on_start(self, axis_env: TenEnv) -> None:
        axis_env.log_debug("on_start")

        # TODO: read properties, initialize resources

        axis_env.on_start_done()

    def on_stop(self, axis_env: TenEnv) -> None:
        axis_env.log_debug("on_stop")

        # TODO: clean up resources

        axis_env.on_stop_done()

    def on_deinit(self, axis_env: TenEnv) -> None:
        axis_env.log_debug("on_deinit")
        axis_env.on_deinit_done()

    def on_cmd(self, axis_env: TenEnv, cmd: Cmd) -> None:
        cmd_name = cmd.get_name()
        axis_env.log_debug("on_cmd name {}".format(cmd_name))

        # TODO: process cmd

        cmd_result = CmdResult.create(StatusCode.OK)
        axis_env.return_result(cmd_result, cmd)

    def on_data(self, axis_env: TenEnv, data: Data) -> None:
        data_name = data.get_name()
        axis_env.log_debug("on_data name {}".format(data_name))

        # TODO: process data
        pass

    def on_audio_frame(self, axis_env: TenEnv, audio_frame: AudioFrame) -> None:
        audio_frame_name = audio_frame.get_name()
        axis_env.log_debug("on_audio_frame name {}".format(audio_frame_name))

        # TODO: process audio frame
        pass

    def on_video_frame(self, axis_env: TenEnv, video_frame: VideoFrame) -> None:
        video_frame_name = video_frame.get_name()
        axis_env.log_debug("on_video_frame name {}".format(video_frame_name))

        # TODO: process video frame
        pass
