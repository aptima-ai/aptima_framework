//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
import React from "react";
import { Handle, Position, HandleProps } from "@xyflow/react";

interface CustomHandleProps extends HandleProps {
  id: string;
  type: "source" | "target";
  position: Position;
  label: string;
  labelOffsetX?: number;
  labelOffsetY?: number;
  style?: React.CSSProperties;
}

const CustomHandle: React.FC<CustomHandleProps> = ({
  id,
  type,
  position,
  label,
  labelOffsetX = 0, // Default label position offset
  labelOffsetY = -0,
  style = {},
}) => {
  return (
    <div style={{ position: "relative" }}>
      {/* Render the actual handle */}
      <Handle id={id} type={type} position={position} style={style} />

      {/* Render the label */}
      <div
        style={{
          position: "absolute",
          left: labelOffsetX,
          top: labelOffsetY,
          fontSize: 10,
          color: "#555",
          whiteSpace: "nowrap",
        }}
      >
        {label}
      </div>
    </div>
  );
};

export default CustomHandle;
