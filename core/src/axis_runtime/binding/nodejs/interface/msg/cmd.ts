//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
import axis_addon from "../axis_addon";
import { Msg } from "./msg";

export class Cmd extends Msg {
  private constructor(name: string, createShellOnly: boolean) {
    super();

    if (createShellOnly) {
      return;
    }

    axis_addon.axis_nodejs_cmd_create(this, name);
  }

  static Create(name: string): Cmd {
    return new Cmd(name, false);
  }
}

axis_addon.axis_nodejs_cmd_register_class(Cmd);
