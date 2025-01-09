#
# Copyright © 2025 Agora
# This file is part of TEN Framework, an open source project.
# Licensed under the Apache License, Version 2.0, with certain conditions.
# Refer to the "LICENSE" file in the root directory for more information.
#
from typing import Optional

from ten import (AudioFrame, Cmd, CmdResult, Data, Extension, StatusCode,
                 TenEnv, TenError, VideoFrame)


class DefaultExtension(Extension):
    def __init__(self, name: str) -> None:
        super().__init__(name)
        self.name = name

    def on_configure(self, aptima_env: TenEnv) -> None:
        aptima_env.log_debug(f"DefaultExtension on_init, name: {self.name}")
        assert self.name == "default_extension_python"

        aptima_env.init_property_from_json('{"testKey": "testValue"}')
        aptima_env.on_configure_done()

    def handle_error(self, aptima_env: TenEnv, error: Optional[TenError]) -> None:
        assert error is not None
        aptima_env.log_error("DefaultExtension handle_error: " + error.err_msg())

        self.no_dest_error_recv_count += 1
        if self.no_dest_error_recv_count == 4:
            aptima_env.on_start_done()

    def on_start(self, aptima_env: TenEnv) -> None:
        aptima_env.log_debug("on_start")

        self.no_dest_error_recv_count = 0

        aptima_env.set_property_from_json("testKey2", '"testValue2"')
        testValue = aptima_env.get_property_to_json("testKey")
        testValue2 = aptima_env.get_property_to_json("testKey2")
        print("testValue: ", testValue, " testValue2: ", testValue2)

        # Send an unconnected command
        cmd = Cmd.create("unconnected_cmd")
        aptima_env.send_cmd(
            cmd,
            lambda aptima_env, result, error: self.handle_error(aptima_env, error),
        )

        # Send an unconnected data
        data = Data.create("unconnected_data")
        aptima_env.send_data(
            data,
            lambda aptima_env, error: self.handle_error(aptima_env, error),
        )

        # Send an unconnected video frame
        video_frame = VideoFrame.create("unconnected_video_frame")
        aptima_env.send_video_frame(
            video_frame,
            lambda aptima_env, error: self.handle_error(aptima_env, error),
        )

        # Send an unconnected audio frame
        audio_frame = AudioFrame.create("unconnected_audio_frame")
        aptima_env.send_audio_frame(
            audio_frame,
            lambda aptima_env, error: self.handle_error(aptima_env, error),
        )

    def on_stop(self, aptima_env: TenEnv) -> None:
        print("DefaultExtension on_stop")
        aptima_env.on_stop_done()

    def on_deinit(self, aptima_env: TenEnv) -> None:
        print("DefaultExtension on_deinit")
        aptima_env.on_deinit_done()

    def check_hello(
        self,
        aptima_env: TenEnv,
        result: Optional[CmdResult],
        error: Optional[TenError],
        receivedCmd: Cmd,
    ):
        if error is not None:
            assert False, error.err_msg()

        assert result is not None

        statusCode = result.get_status_code()
        detail = result.get_property_string("detail")
        print(
            "DefaultExtension check_hello: status:"
            + str(statusCode)
            + " detail:"
            + detail
        )

        respCmd = CmdResult.create(StatusCode.OK)
        respCmd.set_property_string("detail", detail + " nbnb")
        print("DefaultExtension create respCmd")

        aptima_env.return_result(respCmd, receivedCmd)

    def on_cmd(self, aptima_env: TenEnv, cmd: Cmd) -> None:
        print("DefaultExtension on_cmd")

        cmd_json = cmd.get_property_to_json()
        print("DefaultExtension on_cmd json: " + cmd_json)

        new_cmd = Cmd.create("hello")
        new_cmd.set_property_from_json("test", '"testValue2"')
        test_value = new_cmd.get_property_to_json("test")
        print("DefaultExtension on_cmd test_value: " + test_value)

        aptima_env.send_cmd(
            new_cmd,
            lambda aptima_env, result, error: self.check_hello(
                aptima_env, result, error, cmd
            ),
        )
