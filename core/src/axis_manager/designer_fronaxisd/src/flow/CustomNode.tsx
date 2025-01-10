//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
import * as React from "react";
import { Position, NodeProps, Connection, Edge, Node } from "@xyflow/react";
import {
  BlocksIcon as ExtensionIcon,
  LogsIcon,
  CableIcon,
  InfoIcon,
} from "lucide-react";

import { cn } from "@/lib/utils";
import { dispatchCustomNodeActionPopup } from "@/utils/popup";
import CustomHandle from "@/flow/CustomHandle";

const onConnect = (params: Connection | Edge) =>
  console.log("Handle onConnect", params);

export type CustomNodeType = Node<
  {
    name: string;
    addon: string;
    sourceCmds: string[];
    targetCmds: string[];
    sourceData: string[];
    targetData: string[];
    sourceAudioFrame: string[];
    targetAudioFrame: string[];
    sourceVideoFrame: string[];
    targetVideoFrame: string[];
    url?: string;
  },
  "customNode"
>;

export function CustomNode({ data, isConnectable }: NodeProps<CustomNodeType>) {
  return (
    <>
      <div
        className={cn(
          "absolute top-0 -right-3 -translate-y-1/2 z-[1] flex gap-1"
        )}
      >
        <div
          className={cn(
            "gradient",
            "w-6 h-6 p-0.5",
            "rounded-full flex transform overflow-hidden z-[1]",
            "shadow-lg dark:shadow-[0_0_15px_rgba(255,255,255,0.2)]",
            "text-popover-foreground"
          )}
        >
          <div
            className={cn(
              "relative bg-popover text-popover-foreground",
              "flex-grow flex justify-center items-center rounded-full",
              "cursor-pointer"
            )}
            onClick={() => {
              dispatchCustomNodeActionPopup("connections", data.name);
            }}
          >
            <CableIcon className="w-3 h-3" />
          </div>
        </div>

        <div
          className={cn(
            "gradient",
            "w-6 h-6 p-0.5",
            "rounded-full flex transform overflow-hidden z-[1]",
            "shadow-lg dark:shadow-[0_0_15px_rgba(255,255,255,0.2)]",
            "text-popover-foreground"
          )}
        >
          <div
            className={cn(
              "relative bg-popover text-popover-foreground",
              "flex-grow flex justify-center items-center rounded-full"
            )}
            onClick={() => {
              console.log("clicked LogsIcon === ", data);
            }}
          >
            <LogsIcon className="w-3 h-3" />
          </div>
        </div>

        <div
          className={cn(
            "gradient",
            "w-6 h-6 p-0.5",
            "rounded-full flex transform overflow-hidden z-[1]",
            "shadow-lg dark:shadow-[0_0_15px_rgba(255,255,255,0.2)]",
            "text-popover-foreground"
          )}
        >
          <div
            className={cn(
              "relative bg-popover text-popover-foreground",
              "flex-grow flex justify-center items-center rounded-full"
            )}
            onClick={() => {
              console.log("clicked InfoIcon === ", data);
            }}
          >
            <InfoIcon className="w-3 h-3" />
          </div>
        </div>
      </div>

      <div className="wrapper gradient">
        <div
          className={cn(
            "flex flex-col justify-center flex-grow-1",
            "bg-popover px-5 py-4 rounded-lg relative"
          )}
        >
          <div className="flex">
            <div className="mr-2">
              <ExtensionIcon className="h-4 w-4" />
            </div>
            <div>
              <div className="text-base mb-0.5 leading-none">{data.name}</div>
              <div className="text-xs text-muted-foreground">{data.addon}</div>
            </div>
          </div>

          {/* Render target handles (for incoming edges) */}
          {/* {data.targetCmds.map((cmd, index) => (
            <CustomHandle
              key={`target-${cmd}`}
              type="target"
              position={Position.Left}
              id={`target-${cmd}`}
              label={cmd}
              labelOffsetX={0}
              // labelOffsetY={index * 20}
              // style={{ top: index * 20, background: "#555" }}
              isConnectable={isConnectable}
              onConnect={onConnect}
            />
          ))} */}
          <CustomHandle
            key={`target-${data.addon}`}
            type="target"
            position={Position.Left}
            id={`target-${data.addon}`}
            label={data.addon}
            labelOffsetX={0}
            // labelOffsetY={index * 20}
            // style={{ top: index * 20, background: "#555" }}
            isConnectable={isConnectable}
            onConnect={onConnect}
          />

          {/* Render source handles (for outgoing edges) */}
          {/* {data.sourceCmds.map((cmd, index) => (
            <CustomHandle
              key={`source-${cmd}`}
              type="source"
              position={Position.Right}
              id={`source-${cmd}`}
              label={cmd}
              labelOffsetX={0}
              // labelOffsetY={index * 20}
              // style={{ top: index * 20, background: "#555" }}
              isConnectable={isConnectable}
            />
          ))} */}
          <CustomHandle
            key={`source-${data.addon}`}
            type="source"
            position={Position.Right}
            id={`source-${data.addon}`}
            label={data.addon}
            labelOffsetX={0}
            // labelOffsetY={index * 20}
            // style={{ top: index * 20, background: "#555" }}
            isConnectable={isConnectable}
          />
        </div>
      </div>
    </>
  );
}

export default React.memo(CustomNode);
