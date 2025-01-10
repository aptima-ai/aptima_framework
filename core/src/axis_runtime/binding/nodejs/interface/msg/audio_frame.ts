//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
import axis_addon from "../axis_addon";
import { Msg } from "./msg";

export enum AudioFrameDataFmt {
  INTERLEAVE = 1,
  NON_INTERLEAVE = 2,
}

export class AudioFrame extends Msg {
  private constructor(name: string, createShellOnly: boolean) {
    super();

    if (createShellOnly) {
      return;
    }

    axis_addon.axis_nodejs_audio_frame_create(this, name);
  }

  static Create(name: string): AudioFrame {
    return new AudioFrame(name, false);
  }

  allocBuf(size: number): void {
    axis_addon.axis_nodejs_audio_frame_alloc_buf(this, size);
  }

  lockBuf(): ArrayBuffer {
    return axis_addon.axis_nodejs_audio_frame_lock_buf(this);
  }

  unlockBuf(buf: ArrayBuffer): void {
    axis_addon.axis_nodejs_audio_frame_unlock_buf(this, buf);
  }

  getBuf(): ArrayBuffer {
    return axis_addon.axis_nodejs_audio_frame_get_buf(this);
  }

  getTimestamp(): number {
    return axis_addon.axis_nodejs_audio_frame_get_timestamp(this);
  }

  setTimestamp(timestamp: number): void {
    axis_addon.axis_nodejs_audio_frame_set_timestamp(this, timestamp);
  }

  getSampleRate(): number {
    return axis_addon.axis_nodejs_audio_frame_get_sample_rate(this);
  }

  setSampleRate(sampleRate: number): void {
    axis_addon.axis_nodejs_audio_frame_set_sample_rate(this, sampleRate);
  }

  getSamplesPerChannel(): number {
    return axis_addon.axis_nodejs_audio_frame_get_samples_per_channel(this);
  }

  setSamplesPerChannel(samplesPerChannel: number): void {
    axis_addon.axis_nodejs_audio_frame_set_samples_per_channel(
      this,
      samplesPerChannel
    );
  }

  getBytesPerSample(): number {
    return axis_addon.axis_nodejs_audio_frame_get_bytes_per_sample(this);
  }

  setBytesPerSample(bytesPerSample: number): void {
    axis_addon.axis_nodejs_audio_frame_set_bytes_per_sample(this, bytesPerSample);
  }

  getNumberOfChannels(): number {
    return axis_addon.axis_nodejs_audio_frame_get_number_of_channels(this);
  }

  setNumberOfChannels(numberOfChannels: number): void {
    axis_addon.axis_nodejs_audio_frame_set_number_of_channels(
      this,
      numberOfChannels
    );
  }

  getDataFmt(): AudioFrameDataFmt {
    return axis_addon.axis_nodejs_audio_frame_get_data_fmt(this);
  }

  setDataFmt(dataFmt: AudioFrameDataFmt): void {
    axis_addon.axis_nodejs_audio_frame_set_data_fmt(this, dataFmt);
  }

  getLineSize(): number {
    return axis_addon.axis_nodejs_audio_frame_get_line_size(this);
  }

  setLineSize(lineSize: number): void {
    axis_addon.axis_nodejs_audio_frame_set_line_size(this, lineSize);
  }

  isEof(): boolean {
    return axis_addon.axis_nodejs_audio_frame_is_eof(this);
  }

  setEof(eof: boolean): void {
    axis_addon.axis_nodejs_audio_frame_set_eof(this, eof);
  }
}

axis_addon.axis_nodejs_audio_frame_register_class(AudioFrame);
