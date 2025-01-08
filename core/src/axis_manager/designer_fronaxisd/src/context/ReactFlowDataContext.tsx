//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
import * as React from "react";

import type { CustomNodeType } from "@/flow/CustomNode";
import type { CustomEdgeType } from "@/flow/CustomEdge";
import { EConnectionType } from "@/types/graphs";

export type TReactFlowDataContext = {
  nodes: CustomNodeType[];
  edges: CustomEdgeType[];
};

export const ReactFlowDataContext = React.createContext<TReactFlowDataContext>({
  nodes: [],
  edges: [],
});

// --- hooks ---
export const useCDAVInfoByEdgeId = (edgeId: string) => {
  const { nodes, edges } = React.useContext(ReactFlowDataContext);

  const edge = edges.find((e) => e.id === edgeId);
  if (!edge) return null;

  const source = nodes.find((n) => n.id === edge.source);
  const target = nodes.find((n) => n.id === edge.target);
  if (!source || !target) return null;

  // Find all edges between the same source and target nodes
  const relatedEdges = edges.filter(
    (e) => e.source === edge.source && e.target === edge.target
  );

  // Count connections between source and target nodes
  let cmdCount = 0;
  let dataCount = 0;
  let audioCount = 0;
  let videoCount = 0;

  // Count all connection types from related edges
  relatedEdges.forEach((e) => {
    switch (e.data?.connectionType) {
      case EConnectionType.CMD:
        cmdCount++;
        break;
      case EConnectionType.DATA:
        dataCount++;
        break;
      case EConnectionType.AUDIO_FRAME:
        audioCount++;
        break;
      case EConnectionType.VIDEO_FRAME:
        videoCount++;
        break;
    }
  });

  return {
    source,
    target,
    relatedEdges,
    connectionCounts: {
      cmd: cmdCount,
      data: dataCount,
      audio: audioCount,
      video: videoCount,
    },
  };
};
