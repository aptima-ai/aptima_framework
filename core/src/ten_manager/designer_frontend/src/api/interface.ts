//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
export interface SuccessResponse<T> {
  status: "ok";
  data: T;
  meta?: unknown;
}

export interface ErrorResponse {
  status: "fail";
  message: string;
  error?: InnerError;
}

export interface InnerError {
  // Note: The value of 'type' must adhere to the 'error_type' in rust.
  type: string;

  code?: string;
  message: string;
}

export type ApiResponse<T> = SuccessResponse<T> | ErrorResponse;

export interface BackendNode {
  addon: string;
  name: string;
  extension_group: string;
  app: string;
  property: unknown;
  api?: unknown;
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

export interface FileContentResponse {
  content: string;
}

export interface SaveFileRequest {
  content: string;
}

export interface Graph {
  name: string;
  auto_start: boolean;
}

export interface SetBaseDirRequest {
  base_dir: string;
}

export interface SetBaseDirResponse {
  success: boolean;
}
