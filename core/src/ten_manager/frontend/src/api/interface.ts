//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
export interface ApiResponse<T> {
  status: string;
  data: T;
  meta?: any;
}

export interface BackendNode {
  addon: string;
  name: string;
  extension_group: string;
  app: string;
  property: any;
  api?: any;
}

export interface BackendConnection {
  app: string;
  extension_group: string;
  extension: string;
  cmd?: {
    name: string;
    dest: {
      app: string;
      extension_group: string;
      extension: string;
    }[];
  }[];
}
