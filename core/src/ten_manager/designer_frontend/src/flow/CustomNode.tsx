//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
import { memo } from "react";
import { Position, NodeProps, Connection, Edge, Node } from "@xyflow/react";

import CustomHandle from "./CustomHandle";

const onConnect = (params: Connection | Edge) =>
  console.log("Handle onConnect", params);

export type CustomNodeType = Node<
  {
    name: string;
    addon: string;
    sourceCmds: string[];
    targetCmds: string[];
    url?: string;
  },
  "customNode"
>;

export function CustomNode({ data, isConnectable }: NodeProps<CustomNodeType>) {
  return (
    <div>
      <div>{data.name}</div>

      {/* Render source handles (for outgoing edges) */}
      {data.sourceCmds.map((cmd, index) => {
        return (
          <CustomHandle
            key={`source-${cmd}`}
            type="source"
            position={Position.Right}
            id={`source-${cmd}`}
            label={cmd}
            labelOffsetX={0}
            labelOffsetY={index * 20}
            style={{ top: index * 20, background: "#555" }}
            isConnectable={isConnectable}
          />
        );
      })}

      {/* Render target handles (for incoming edges) */}
      {data.targetCmds.map((cmd, index) => {
        return (
          <CustomHandle
            key={`target-${cmd}`}
            type="target"
            position={Position.Left}
            id={`target-${cmd}`}
            label={cmd}
            labelOffsetX={0}
            labelOffsetY={index * 20}
            style={{ top: index * 20, background: "#555" }}
            isConnectable={isConnectable}
            onConnect={onConnect}
          />
        );
      })}
    </div>
  );
}

export default memo(CustomNode);
