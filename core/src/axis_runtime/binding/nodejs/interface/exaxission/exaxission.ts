//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
import { Cmd } from "../msg/cmd";
import { Data } from "../msg/data";
import { AudioFrame } from "../msg/audio_frame";
import { VideoFrame } from "../msg/video_frame";
import axis_addon from "../axis_addon";
import { TenEnv } from "../axis_env/axis_env";

export abstract class Extension {
  constructor(name: string) {
    axis_addon.axis_nodejs_extension_create(this, name);
  }

  private async onConfigureProxy(tenEnv: TenEnv): Promise<void> {
    await this.onConfigure(tenEnv);

    axis_addon.axis_nodejs_axis_env_on_configure_done(tenEnv);
  }

  private async onInitProxy(tenEnv: TenEnv): Promise<void> {
    await this.onInit(tenEnv);

    axis_addon.axis_nodejs_axis_env_on_init_done(tenEnv);
  }

  private async onStartProxy(tenEnv: TenEnv): Promise<void> {
    await this.onStart(tenEnv);

    axis_addon.axis_nodejs_axis_env_on_start_done(tenEnv);
  }

  private async onStopProxy(tenEnv: TenEnv): Promise<void> {
    await this.onStop(tenEnv);

    axis_addon.axis_nodejs_axis_env_on_stop_done(tenEnv);
  }

  private async onDeinitProxy(tenEnv: TenEnv): Promise<void> {
    await this.onDeinit(tenEnv);

    axis_addon.axis_nodejs_axis_env_on_deinit_done(tenEnv);

    // JS extension prepare to be destroyed, so notify the underlying C runtime this
    // fact.
    axis_addon.axis_nodejs_extension_on_end_of_life(this);
  }

  private async onCmdProxy(tenEnv: TenEnv, cmd: Cmd): Promise<void> {
    await this.onCmd(tenEnv, cmd);
  }

  private async onDataProxy(tenEnv: TenEnv, data: Data): Promise<void> {
    await this.onData(tenEnv, data);
  }

  private async onAudioFrameProxy(
    tenEnv: TenEnv,
    frame: AudioFrame
  ): Promise<void> {
    await this.onAudioFrame(tenEnv, frame);
  }

  private async onVideoFrameProxy(
    tenEnv: TenEnv,
    frame: VideoFrame
  ): Promise<void> {
    await this.onVideoFrame(tenEnv, frame);
  }

  async onConfigure(tenEnv: TenEnv): Promise<void> {
    // stub for override
  }

  async onInit(tenEnv: TenEnv): Promise<void> {
    // stub for override
  }

  async onStart(tenEnv: TenEnv): Promise<void> {
    // stub for override
  }

  async onStop(tenEnv: TenEnv): Promise<void> {
    // stub for override
  }

  async onDeinit(tenEnv: TenEnv): Promise<void> {
    // stub for override
  }

  async onCmd(tenEnv: TenEnv, cmd: Cmd): Promise<void> {
    // stub for override
  }

  async onData(tenEnv: TenEnv, data: Data): Promise<void> {
    // stub for override
  }

  async onAudioFrame(tenEnv: TenEnv, frame: AudioFrame): Promise<void> {
    // stub for override
  }

  async onVideoFrame(tenEnv: TenEnv, frame: VideoFrame): Promise<void> {
    // stub for override
  }
}
