//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
import { EdgeProps, getBezierPath, Edge } from "@xyflow/react";

export type CustomEdgeType = Edge<
  { labelOffsetX: number; labelOffsetY: number },
  "customEdge"
>;

export function CustomEdge({
  sourceX,
  sourceY,
  targetX,
  targetY,
  sourcePosition,
  targetPosition,
  data,
  id,
  style,
  label,
}: EdgeProps<CustomEdgeType>) {
  const [edgePath, labelX, labelY] = getBezierPath({
    sourceX,
    sourceY,
    targetX,
    targetY,
    sourcePosition,
    targetPosition,
  });

  // Customize label position offset for left alignment.
  // Adjust to move label closer to the left.
  const labelOffsetX = data?.labelOffsetX || -50;
  // Adjust for vertical positioning.
  const labelOffsetY = data?.labelOffsetY || -10;

  // Compute position closer to the source point.
  // 25% from source.
  const leftLabelX = sourceX + (labelX - sourceX) * 0.25 + labelOffsetX;
  const leftLabelY = sourceY + (labelY - sourceY) * 0.25 + labelOffsetY;

  return (
    <>
      <path
        id={id}
        style={style}
        className="react-flow__edge-path"
        d={edgePath}
      />
      {label && (
        <text
          x={leftLabelX}
          y={leftLabelY}
          textAnchor="start"
          style={{ fontSize: 12, fill: "#000" }}
        >
          {label}
        </text>
      )}
    </>
  );
}

export default CustomEdge;
