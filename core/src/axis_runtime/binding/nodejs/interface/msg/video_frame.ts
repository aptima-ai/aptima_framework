//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
import axis_addon from "../axis_addon";
import { Msg } from "./msg";

export enum PixelFmt {
  RGB24 = 1,
  RGBA = 2,
  BGR24 = 3,
  BGRA = 4,
  I422 = 5,
  I420 = 6,
  NV21 = 7,
  NV12 = 8,
}

export class VideoFrame extends Msg {
  private constructor(name: string, createShellOnly: boolean) {
    super();

    if (createShellOnly) {
      return;
    }

    axis_addon.axis_nodejs_video_frame_create(this, name);
  }

  static Create(name: string): VideoFrame {
    return new VideoFrame(name, false);
  }

  allocBuf(size: number): void {
    axis_addon.axis_nodejs_video_frame_alloc_buf(this, size);
  }

  lockBuf(): ArrayBuffer {
    return axis_addon.axis_nodejs_video_frame_lock_buf(this);
  }

  unlockBuf(buf: ArrayBuffer): void {
    axis_addon.axis_nodejs_video_frame_unlock_buf(this, buf);
  }

  getBuf(): ArrayBuffer {
    return axis_addon.axis_nodejs_video_frame_get_buf(this);
  }

  getWidth(): number {
    return axis_addon.axis_nodejs_video_frame_get_width(this);
  }

  setWidth(width: number): void {
    axis_addon.axis_nodejs_video_frame_set_width(this, width);
  }

  getHeight(): number {
    return axis_addon.axis_nodejs_video_frame_get_height(this);
  }

  setHeight(height: number): void {
    axis_addon.axis_nodejs_video_frame_set_height(this, height);
  }

  getTimestamp(): number {
    return axis_addon.axis_nodejs_video_frame_get_timestamp(this);
  }

  setTimestamp(timestamp: number): void {
    axis_addon.axis_nodejs_video_frame_set_timestamp(this, timestamp);
  }

  getPixelFmt(): PixelFmt {
    return axis_addon.axis_nodejs_video_frame_get_pixel_fmt(this);
  }

  setPixelFmt(pixelFmt: PixelFmt): void {
    axis_addon.axis_nodejs_video_frame_set_pixel_fmt(this, pixelFmt);
  }

  isEof(): boolean {
    return axis_addon.axis_nodejs_video_frame_is_eof(this);
  }

  setEof(eof: boolean): void {
    axis_addon.axis_nodejs_video_frame_set_eof(this, eof);
  }
}

axis_addon.axis_nodejs_video_frame_register_class(VideoFrame);
