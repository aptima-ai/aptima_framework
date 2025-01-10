//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
import {
  type EdgeProps,
  getBezierPath,
  type Edge,
  BaseEdge,
  getEdgeCenter,
} from "@xyflow/react";
import { cn } from "@/lib/utils";

import { useCDAVInfoByEdgeId } from "@/context/ReactFlowDataContext";
import { dispatchCustomNodeActionPopup } from "@/utils/popup";

import { type EConnectionType } from "@/types/graphs";

export type CustomEdgeType = Edge<
  {
    labelOffsetX: number;
    labelOffsetY: number;
    connectionType: EConnectionType;
  },
  "customEdge"
>;

export function CustomEdge({
  sourceX,
  sourceY,
  targetX,
  targetY,
  sourcePosition,
  targetPosition,
  id,
  style,
  selected,
  source,
  target,
}: EdgeProps<CustomEdgeType>) {
  const [edgePath] = getBezierPath({
    sourceX,
    sourceY,
    sourcePosition,
    targetX,
    targetY,
    targetPosition,
  });
  const [edgeCenterX, edgeCenterY] = getEdgeCenter({
    sourceX,
    sourceY,
    targetX,
    targetY,
  });

  const { connectionCounts } = useCDAVInfoByEdgeId(id) ?? {};

  const onEdgeClick = (e: React.MouseEvent<HTMLDivElement>) => {
    e.stopPropagation();
    dispatchCustomNodeActionPopup("connections", source, target);
  };

  return (
    <>
      <BaseEdge id={id} path={edgePath} style={style} />
      {selected && (
        <path
          id={id}
          d={edgePath}
          fill="none"
          strokeDasharray="5,5"
          stroke="url(#edge-gradient)"
          strokeWidth="2"
          opacity="0.75"
        >
          <animate
            attributeName="stroke-dashoffset"
            values="10;0"
            dur="0.75s"
            repeatCount="indefinite"
          />
        </path>
      )}
      <foreignObject
        width={80}
        height={60}
        x={edgeCenterX - 40}
        y={edgeCenterY - 30}
        className="pointer-events-all nodrag nopan"
        requiredExtensions="http://www.w3.org/1999/xhtml"
      >
        <div
          className={cn(
            "cursor-pointer rounded-lg border bg-background",
            "hover:bg-muted text-xs pt-0 px-2 py-1"
          )}
          onClick={(event) => onEdgeClick(event)}
        >
          <div className="grid grid-cols-2 gap-0.5">
            <div className="flex gap-1 justify-center items-center">
              <span className="text-muted-foreground">C:</span>
              {connectionCounts?.cmd ?? 0}
            </div>
            <div className="flex gap-1 justify-center items-center">
              <span className="text-muted-foreground">D:</span>
              {connectionCounts?.data ?? 0}
            </div>
            <div className="flex gap-1 justify-center items-center">
              <span className="text-muted-foreground">A:</span>
              {connectionCounts?.audio ?? 0}
            </div>
            <div className="flex gap-1 justify-center items-center">
              <span className="text-muted-foreground">V:</span>
              {connectionCounts?.video ?? 0}
            </div>
          </div>
        </div>
      </foreignObject>
    </>
  );
}

export default CustomEdge;
