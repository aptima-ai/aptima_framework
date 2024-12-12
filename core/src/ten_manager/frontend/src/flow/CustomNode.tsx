import { memo, CSSProperties } from "react";
import {
  Handle,
  Position,
  NodeProps,
  Connection,
  Edge,
  Node,
} from "@xyflow/react";

const targetHandleStyle: CSSProperties = { background: "#555" };
const sourceHandleStyle: CSSProperties = { ...targetHandleStyle };

const onConnect = (params: Connection | Edge) =>
  console.log("Handle onConnect", params);

export type CustomNodeType = Node<
  { label: string; sourceCmds: string[]; targetCmds: string[] },
  "customNode"
>;

export function CustomNode({ data, isConnectable }: NodeProps<CustomNodeType>) {
  return (
    <div>
      {/* Render target handles (for incoming edges) */}
      {data.targetCmds.map((cmd, index) => {
        return (
          <Handle
            key={`target-${cmd}`}
            type="target"
            position={Position.Left}
            id={`target-${cmd}`}
            style={{ ...targetHandleStyle, top: 10 + index * 15 }}
            isConnectable={isConnectable}
            onConnect={onConnect}
          />
        );
      })}

      <div>{data.label}</div>

      {/* Render source handles (for outgoing edges) */}
      {data.sourceCmds.map((cmd, index) => {
        return (
          <Handle
            key={`source-${cmd}`}
            type="source"
            position={Position.Right}
            id={`source-${cmd}`}
            style={{ ...sourceHandleStyle, top: 10 + index * 15 }}
            isConnectable={isConnectable}
          />
        );
      })}
    </div>
  );
}

export default memo(CustomNode);
