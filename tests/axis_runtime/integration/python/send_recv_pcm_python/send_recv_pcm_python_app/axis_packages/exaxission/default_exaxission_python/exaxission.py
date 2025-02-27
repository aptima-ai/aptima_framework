#
# Copyright © 2025 Agora
# This file is part of APTIMA Framework, an open source project.
# Licensed under the Apache License, Version 2.0, with certain conditions.
# Refer to the "LICENSE" file in the root directory for more information.
#
import time

from pydub import AudioSegment
from aptima import (
    AudioFrame,
    AudioFrameDataFmt,
    Cmd,
    CmdResult,
    Extension,
    StatusCode,
    TenEnv,
)


class DefaultExtension(Extension):
    def on_init(self, aptima_env: TenEnv) -> None:
        aptima_env.log_debug("on_init")
        aptima_env.on_init_done()

    def on_start(self, aptima_env: TenEnv) -> None:
        aptima_env.log_debug("on_start")
        self.audio = AudioSegment.from_file(
            "../test_data/speech_16k_1.pcm",
            format="s16le",
            sample_width=2,
            channels=1,
            frame_rate=16000,
        )
        print("audio duration: ", len(self.audio))

        self.start_time = int(time.time() * 1000)

        aptima_env.on_start_done()

    def on_cmd(self, aptima_env: TenEnv, cmd: Cmd) -> None:
        cmd_json = cmd.get_property_to_json()
        print("DefaultExtension on_cmd json: " + cmd_json)

        self.request_cmd = cmd

        raw_data = self.audio.raw_data
        print("audio raw data len: ", len(raw_data))

        pcm_data = self.audio[0:10].raw_data
        audio_frame = AudioFrame.create("pcm")
        audio_frame.set_data_fmt(AudioFrameDataFmt.INTERLEAVE)
        audio_frame.set_bytes_per_sample(2)
        audio_frame.set_sample_rate(16000)
        audio_frame.set_number_of_channels(1)
        audio_frame.set_samples_per_channel(160)

        timestamp = int(time.time() * 1000)
        audio_frame.set_timestamp(timestamp)

        audio_frame.alloc_buf(len(pcm_data))
        buf = audio_frame.lock_buf()
        buf[: len(pcm_data)] = pcm_data
        audio_frame.unlock_buf(buf)
        aptima_env.send_audio_frame(audio_frame)

    def on_audio_frame(
        self, aptima_env: TenEnv, audio_frame: AudioFrame
    ) -> None:
        print("DefaultExtension on_audio_frame")

        assert audio_frame.get_bytes_per_sample() == 2
        assert audio_frame.get_sample_rate() == 16000
        assert audio_frame.get_number_of_channels() == 1
        assert audio_frame.get_samples_per_channel() == 160

        timestamp = audio_frame.get_timestamp()

        current_time = int(time.time() * 1000)
        assert current_time >= timestamp >= self.start_time

        buf = audio_frame.get_buf()

        assert buf[: len(buf)] == self.audio[0:10].raw_data

        cmd_result = CmdResult.create(StatusCode.OK)
        cmd_result.set_property_string("detail", "success")
        aptima_env.return_result(cmd_result, self.request_cmd)

    def on_stop(self, aptima_env: TenEnv) -> None:
        print("DefaultExtension on_stop")
        aptima_env.on_stop_done()
