//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
import ten_addon from "../ten_addon";
import { Msg } from "./msg";

export enum StatusCode {
  OK = 0,
  ERROR = 1,
}

export class CmdResult extends Msg {
  private constructor(statusCode: StatusCode, createShellOnly: boolean) {
    super();

    if (createShellOnly) {
      return;
    }

    ten_addon.ten_nodejs_cmd_result_create(this, statusCode);
  }

  static Create(statusCode: StatusCode): CmdResult {
    return new CmdResult(statusCode, false);
  }

  getStatusCode(): StatusCode {
    return ten_addon.ten_nodejs_cmd_result_get_status_code(this);
  }

  setFinal(isFinal: boolean): void {
    ten_addon.ten_nodejs_cmd_result_set_final(this, isFinal);
  }

  isFinal(): boolean {
    return ten_addon.ten_nodejs_cmd_result_is_final(this);
  }

  isCompleted(): boolean {
    return ten_addon.ten_nodejs_cmd_result_is_completed(this);
  }
}

ten_addon.ten_nodejs_cmd_result_register_class(CmdResult);
