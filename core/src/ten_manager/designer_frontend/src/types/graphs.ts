//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
export interface IBackendNode {
  addon: string;
  name: string;
  extension_group: string;
  app: string;
  property: unknown;
  api?: unknown;
}

export interface IBackendConnection {
  app: string;
  extension: string;
  cmd?: {
    name: string;
    dest: {
      app: string;
      extension: string;
    }[];
  }[];
}

export interface IGraph {
  name: string;
  auto_start: boolean;
}
