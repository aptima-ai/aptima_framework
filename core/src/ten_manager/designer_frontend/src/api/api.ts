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
  SuccessResponse,
} from "./interface";

export interface ExtensionAddon {
  addon_name: string;
  url: string;
  api?: any;
}

export const fetchNodes = async (): Promise<BackendNode[]> => {
  const response = await fetch(`/api/designer/v1/graphs/default/nodes`);
  if (!response.ok) {
    throw new Error(`Failed to fetch nodes: ${response.status}`);
  }
  const data: ApiResponse<BackendNode[]> = await response.json();

  if (!isSuccessResponse(data)) {
    throw new Error(`Error fetching nodes: ${data.message}`);
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
    throw new Error(`Error fetching connections: ${data.message}`);
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
    throw new Error(`Error fetching addon '${name}': ${data.message}`);
  }
};

export const isSuccessResponse = <T>(
  response: ApiResponse<T>
): response is SuccessResponse<T> => {
  return response.status === "ok";
};
