//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
export interface IFileContentResponse {
  content: string;
}

export interface ISetBaseDirResponse {
  success: boolean;
}

export type TBaseDirEntry = {
  name: string;
  path: string;
  is_dir?: boolean;
};

export interface IBaseDirResponse {
  entries: TBaseDirEntry[];
}
