//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
import z from "zod";

import { API_DESIGNER_V1, ENDPOINT_METHOD } from "@/api/endpoints/constant";
import { genResSchema } from "@/api/endpoints/utils";
import type { IBackendNode, IBackendConnection, IGraph } from "@/types/graphs";

export const ENDPOINT_GRAPHS = {
  nodes: {
    [ENDPOINT_METHOD.GET]: {
      url: `${API_DESIGNER_V1}/graphs/:graphName/nodes`,
      method: ENDPOINT_METHOD.GET,
      pathParams: ["graphName"],
      responseSchema: genResSchema<IBackendNode[]>(
        z.array(
          z.object({
            addon: z.string(),
            name: z.string(),
            extension_group: z.string(),
            app: z.string(),
            property: z.unknown(), // Required property
            api: z.unknown().optional(), // Optional property
          })
        ) as z.ZodType<IBackendNode[]>
      ),
    },
  },
  connections: {
    [ENDPOINT_METHOD.GET]: {
      url: `${API_DESIGNER_V1}/graphs/:graphName/connections`,
      method: ENDPOINT_METHOD.GET,
      pathParams: ["graphName"],
      responseSchema: genResSchema<IBackendConnection[]>(
        z.array(
          z.object({
            app: z.string(),
            extension: z.string(),
            cmd: z
              .array(
                z.object({
                  name: z.string(),
                  dest: z.array(
                    z.object({
                      app: z.string(),
                      extension: z.string(),
                    })
                  ),
                })
              )
              .optional(),
          })
        )
      ),
    },
  },
  graphs: {
    [ENDPOINT_METHOD.GET]: {
      url: `${API_DESIGNER_V1}/graphs`,
      method: ENDPOINT_METHOD.GET,
      responseSchema: genResSchema<IGraph[]>(
        z.array(
          z.object({
            name: z.string(),
            auto_start: z.boolean(),
          })
        )
      ),
    },
  },
};
