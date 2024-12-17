//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
import {
  ApiResponse,
  BackendConnection,
  BackendNode,
  FileContentResponse,
  SaveFileRequest,
  SuccessResponse,
} from "./interface";

export interface ExtensionAddon {
  addon_name: string;
  url: string;
  api?: any;
}

export const isSuccessResponse = <T>(
  response: ApiResponse<T>
): response is SuccessResponse<T> => {
  return response.status === "ok";
};

export const fetchNodes = async (): Promise<BackendNode[]> => {
  const response = await fetch(`/api/designer/v1/graphs/default/nodes`);
  if (!response.ok) {
    throw new Error(`Failed to fetch nodes: ${response.status}`);
  }
  const data: ApiResponse<BackendNode[]> = await response.json();

  if (!isSuccessResponse(data)) {
    throw new Error(`Failed to fetch nodes: ${data.message}`);
  }

  return data.data;
};

export const fetchConnections = async (): Promise<BackendConnection[]> => {
  const response = await fetch(`/api/designer/v1/graphs/default/connections`);
  if (!response.ok) {
    throw new Error(`Failed to fetch connections: ${response.status}`);
  }
  const data: ApiResponse<BackendConnection[]> = await response.json();

  if (!isSuccessResponse(data)) {
    throw new Error(`Failed to fetch connection: ${data.message}`);
  }

  return data.data;
};

/**
 * Fetch extension addon information by name.
 * @param name - The name of the extension addon.
 * @returns The extension addon information.
 */
export const fetchExtensionAddonByName = async (
  name: string
): Promise<ExtensionAddon> => {
  const response = await fetch(`/api/designer/v1/addons/extensions/${name}`);
  if (!response.ok) {
    throw new Error(`Failed to fetch addon '${name}': ${response.status}`);
  }

  const data: ApiResponse<ExtensionAddon> = await response.json();

  if (isSuccessResponse(data)) {
    return data.data;
  } else {
    throw new Error(`Failed to fetch addon '${name}': ${data.message}`);
  }
};

// Get the contents of the file at the specified path.
export const getFileContent = async (
  path: string
): Promise<FileContentResponse> => {
  const encodedPath = encodeURIComponent(path);
  const response = await fetch(`/api/designer/v1/file-content/${encodedPath}`, {
    method: "GET",
    headers: {
      "Content-Type": "application/json",
    },
  });

  if (!response.ok) {
    throw new Error(`Failed to fetch file content: ${response.status}`);
  }

  const data: ApiResponse<FileContentResponse> = await response.json();

  if (!isSuccessResponse(data)) {
    throw new Error(`Failed to fetch file content: ${data.status}`);
  }

  return data.data;
};

// Save the contents of the file at the specified path.
export const saveFileContent = async (
  path: string,
  content: string
): Promise<void> => {
  const encodedPath = encodeURIComponent(path);
  const body: SaveFileRequest = { content };

  const response = await fetch(`/api/designer/v1/file-content/${encodedPath}`, {
    method: "PUT",
    headers: {
      "Content-Type": "application/json",
    },
    body: JSON.stringify(body),
  });

  if (!response.ok) {
    throw new Error(`Failed to save file content: ${response.status}`);
  }

  const data: ApiResponse<null> = await response.json();

  if (data.status !== "ok") {
    throw new Error(`Failed to save file content: ${data.status}`);
  }
};
