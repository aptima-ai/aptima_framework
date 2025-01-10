//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
import axis_addon from "../axis_addon";
import { Msg } from "./msg";

export class Data extends Msg {
  private constructor(name: string, createShellOnly: boolean) {
    super();

    if (createShellOnly) {
      return;
    }

    axis_addon.axis_nodejs_data_create(this, name);
  }

  static Create(name: string): Data {
    return new Data(name, false);
  }

  allocBuf(size: number): void {
    axis_addon.axis_nodejs_data_alloc_buf(this, size);
  }

  lockBuf(): ArrayBuffer {
    return axis_addon.axis_nodejs_data_lock_buf(this);
  }

  unlockBuf(buf: ArrayBuffer): void {
    axis_addon.axis_nodejs_data_unlock_buf(this, buf);
  }

  getBuf(): ArrayBuffer {
    return axis_addon.axis_nodejs_data_get_buf(this);
  }
}

axis_addon.axis_nodejs_data_register_class(Data);
