#
# This file is part of TEN Framework, an open source project.
# Licensed under the Apache License, Version 2.0.
# See the LICENSE file for more information.
#
from ten import (
    AudioFrame,
    VideoFrame,
    AsyncExtension,
    AsyncTenEnv,
    Cmd,
    StatusCode,
    CmdResult,
    Data,
)


class DefaultAsyncExtension(AsyncExtension):
    async def on_init(self, axis_env: AsyncTenEnv) -> None:
        axis_env.log_debug("on_init")

    async def on_start(self, axis_env: AsyncTenEnv) -> None:
        axis_env.log_debug("on_start")

        # TODO: read properties, initialize resources

    async def on_stop(self, axis_env: AsyncTenEnv) -> None:
        axis_env.log_debug("on_stop")

        # TODO: clean up resources

    async def on_deinit(self, axis_env: AsyncTenEnv) -> None:
        axis_env.log_debug("on_deinit")

    async def on_cmd(self, axis_env: AsyncTenEnv, cmd: Cmd) -> None:
        cmd_name = cmd.get_name()
        axis_env.log_debug("on_cmd name {}".format(cmd_name))

        if cmd_name == "query_weather":
            # Send a command to query weather.
            query_cmd = Cmd.create("query_weather")
            cmd_result, error = await axis_env.send_cmd(query_cmd)
            assert error is None
            assert cmd_result is not None

            # Get the weather detail.
            weather = cmd_result.get_property_string("detail")

            # Return the weather detail.
            cmd_result = CmdResult.create(StatusCode.OK)
            cmd_result.set_property_string("detail", weather)
            await axis_env.return_result(cmd_result, cmd)

    async def on_data(self, axis_env: AsyncTenEnv, data: Data) -> None:
        data_name = data.get_name()
        axis_env.log_debug("on_data name {}".format(data_name))

        # TODO: process data
        pass

    async def on_audio_frame(
        self, axis_env: AsyncTenEnv, audio_frame: AudioFrame
    ) -> None:
        audio_frame_name = audio_frame.get_name()
        axis_env.log_debug("on_audio_frame name {}".format(audio_frame_name))

        # TODO: process audio frame
        pass

    async def on_video_frame(
        self, axis_env: AsyncTenEnv, video_frame: VideoFrame
    ) -> None:
        video_frame_name = video_frame.get_name()
        axis_env.log_debug("on_video_frame name {}".format(video_frame_name))

        # TODO: process video frame
        pass
