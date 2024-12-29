//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
import {
  ApiResponse,
  BackendConnection,
  BackendNode,
  Graph,
  FileContentResponse,
  SaveFileRequest,
  SuccessResponse,
  SetBaseDirResponse,
  SetBaseDirRequest,
} from "./interface";

export interface ExtensionAddon {
  addon_name: string;
  url: string;
  api?: unknown;
}

export const isSuccessResponse = <T>(
  response: ApiResponse<T>
): response is SuccessResponse<T> => {
  return response.status === "ok";
};

export const fetchDesignerVersion = async (): Promise<string> => {
  const response = await fetch("/api/designer/v1/version");
  if (!response.ok) {
    throw new Error(`Failed to fetch version: ${response.status}`);
  }

  const data: ApiResponse<{ version: string }> = await response.json();

  if (isSuccessResponse(data) && data.data.version) {
    return data.data.version;
  } else {
    throw new Error("Failed to fetch version information.");
  }
};

export const fetchNodes = async (graphName: string): Promise<BackendNode[]> => {
  const encodedGraphName = encodeURIComponent(graphName);
  const response = await fetch(
    `/api/designer/v1/graphs/${encodedGraphName}/nodes`
  );
  if (!response.ok) {
    throw new Error(`Failed to fetch nodes: ${response.status}`);
  }
  const data: ApiResponse<BackendNode[]> = await response.json();

  if (!isSuccessResponse(data)) {
    throw new Error(`Failed to fetch nodes: ${data.message}`);
  }

  return data.data;
};

export const fetchConnections = async (
  graphName: string
): Promise<BackendConnection[]> => {
  const encodedGraphName = encodeURIComponent(graphName);
  const response = await fetch(
    `/api/designer/v1/graphs/${encodedGraphName}/connections`
  );
  if (!response.ok) {
    throw new Error(`Failed to fetch connections: ${response.status}`);
  }
  const data: ApiResponse<BackendConnection[]> = await response.json();

  if (!isSuccessResponse(data)) {
    throw new Error(`Failed to fetch connection: ${data.message}`);
  }

  return data.data;
};

export const fetchGraphs = async (): Promise<Graph[]> => {
  const response = await fetch(`/api/designer/v1/graphs`);
  const data: ApiResponse<Graph[]> = await response.json();

  if (!isSuccessResponse(data)) {
    if (!response.ok) {
      throw new Error(`${data.message}`);
    } else {
      throw new Error("Should not happen.");
    }
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

export const setBaseDir = async (
  baseDir: string
): Promise<SetBaseDirResponse> => {
  const requestBody: SetBaseDirRequest = { base_dir: baseDir };

  const response = await fetch("/api/designer/v1/base-dir", {
    method: "PUT",
    headers: {
      "Content-Type": "application/json",
    },
    body: JSON.stringify(requestBody),
  });

  const data: ApiResponse<SetBaseDirResponse> = await response.json();

  if (!isSuccessResponse(data)) {
    if (!response.ok) {
      throw new Error(`${data.message}`);
    } else {
      throw new Error("Should not happen.");
    }
  }

  return data.data;
};
