//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
import { MarkerType } from "@xyflow/react";
import dagre from "dagre";

import { CustomNodeType } from "@/flow/CustomNode";
import { CustomEdgeType } from "@/flow/CustomEdge";
import { getExtensionAddonByName } from "@/api/services/addons";
import type { IExtensionAddon } from "@/types/addons";
import {
  type IBackendNode,
  type IBackendConnection,
  EConnectionType,
} from "@/types/graphs";

const NODE_WIDTH = 172;
const NODE_HEIGHT = 36;

export const getLayoutedElements = (
  nodes: CustomNodeType[],
  edges: CustomEdgeType[],
  direction = "TB"
) => {
  const dagreGraph = new dagre.graphlib.Graph();
  dagreGraph.setDefaultEdgeLabel(() => ({}));
  dagreGraph.setGraph({ rankdir: direction });

  nodes.forEach((node) => {
    dagreGraph.setNode(node.id, { width: NODE_WIDTH, height: NODE_HEIGHT });
  });

  edges.forEach((edge) => {
    dagreGraph.setEdge(edge.source, edge.target);
  });

  dagre.layout(dagreGraph);

  const layoutedNodes = nodes.map((node) => {
    const nodeWithPosition = dagreGraph.node(node.id);
    return {
      ...node,
      position: {
        x: nodeWithPosition.x - NODE_WIDTH / 2,
        y: nodeWithPosition.y - NODE_HEIGHT / 2,
      },
    };
  });

  return { nodes: layoutedNodes, edges };
};

export const processNodes = (
  backendNodes: IBackendNode[]
): CustomNodeType[] => {
  return backendNodes.map((n, index) => ({
    id: n.name,
    position: { x: index * 200, y: 100 },
    type: "customNode",
    data: {
      name: `${n.name}`,
      addon: n.addon,
      sourceCmds: [],
      targetCmds: [],
      sourceData: [],
      targetData: [],
      sourceAudioFrame: [],
      targetAudioFrame: [],
      sourceVideoFrame: [],
      targetVideoFrame: [],
    },
  }));
};

export const processConnections = (
  backendConnections: IBackendConnection[]
): {
  initialEdges: CustomEdgeType[];
  nodeSourceCmdMap: Record<string, Set<string>>;
  nodeSourceDataMap: Record<string, Set<string>>;
  nodeSourceAudioFrameMap: Record<string, Set<string>>;
  nodeSourceVideoFrameMap: Record<string, Set<string>>;
  nodeTargetCmdMap: Record<string, Set<string>>;
  nodeTargetDataMap: Record<string, Set<string>>;
  nodeTargetAudioFrameMap: Record<string, Set<string>>;
  nodeTargetVideoFrameMap: Record<string, Set<string>>;
} => {
  const initialEdges: CustomEdgeType[] = [];
  const nodeSourceCmdMap: Record<string, Set<string>> = {};
  const nodeSourceDataMap: Record<string, Set<string>> = {};
  const nodeSourceAudioFrameMap: Record<string, Set<string>> = {};
  const nodeSourceVideoFrameMap: Record<string, Set<string>> = {};
  const nodeTargetCmdMap: Record<string, Set<string>> = {};
  const nodeTargetDataMap: Record<string, Set<string>> = {};
  const nodeTargetAudioFrameMap: Record<string, Set<string>> = {};
  const nodeTargetVideoFrameMap: Record<string, Set<string>> = {};

  backendConnections.forEach((c) => {
    const sourceNodeId = c.extension;
    const types = [
      EConnectionType.CMD,
      EConnectionType.DATA,
      EConnectionType.AUDIO_FRAME,
      EConnectionType.VIDEO_FRAME,
    ];
    types.forEach((type) => {
      if (c[type as keyof IBackendConnection]) {
        (
          c[type as keyof IBackendConnection] as Array<{
            name: string;
            dest: Array<{ extension: string }>;
          }>
        ).forEach((item) => {
          item.dest.forEach((d) => {
            const targetNodeId = d.extension;
            const edgeId = `edge-${sourceNodeId}-${item.name}-${targetNodeId}`;
            const itemName = item.name;

            // Record the item name in the appropriate source map based on type
            let sourceMap: Record<string, Set<string>>;
            let targetMap: Record<string, Set<string>>;
            switch (type) {
              case EConnectionType.CMD:
                sourceMap = nodeSourceCmdMap;
                targetMap = nodeTargetCmdMap;
                break;
              case EConnectionType.DATA:
                sourceMap = nodeSourceDataMap;
                targetMap = nodeTargetDataMap;
                break;
              case EConnectionType.AUDIO_FRAME:
                sourceMap = nodeSourceAudioFrameMap;
                targetMap = nodeTargetAudioFrameMap;
                break;
              case EConnectionType.VIDEO_FRAME:
                sourceMap = nodeSourceVideoFrameMap;
                targetMap = nodeTargetVideoFrameMap;
                break;
              default:
                sourceMap = nodeSourceCmdMap;
                targetMap = nodeTargetCmdMap;
            }

            if (!sourceMap[sourceNodeId]) {
              sourceMap[sourceNodeId] = new Set();
            }
            sourceMap[sourceNodeId].add(itemName);

            // Record the item name in the appropriate target map
            if (!targetMap[targetNodeId]) {
              targetMap[targetNodeId] = new Set();
            }
            targetMap[targetNodeId].add(itemName);

            initialEdges.push({
              id: edgeId,
              source: sourceNodeId,
              target: targetNodeId,
              data: {
                connectionType: type,
                labelOffsetX: 0,
                labelOffsetY: 0,
              },
              type: "customEdge",
              label: itemName,
              sourceHandle: `source-${sourceNodeId}`,
              targetHandle: `target-${targetNodeId}`,
              markerEnd: {
                type: MarkerType.ArrowClosed,
              },
            });
          });
        });
      }
    });
  });

  return {
    initialEdges,
    nodeSourceCmdMap,
    nodeSourceDataMap,
    nodeSourceAudioFrameMap,
    nodeSourceVideoFrameMap,
    nodeTargetCmdMap,
    nodeTargetDataMap,
    nodeTargetAudioFrameMap,
    nodeTargetVideoFrameMap,
  };
};

export const fetchAddonInfoForNodes = async (
  nodes: CustomNodeType[]
): Promise<CustomNodeType[]> => {
  return await Promise.all(
    nodes.map(async (node) => {
      try {
        const addonInfo: IExtensionAddon = await getExtensionAddonByName(
          node.data.addon
        );
        console.log(`URL for addon '${node.data.addon}': ${addonInfo.url}`);
        return {
          ...node,
          data: {
            ...node.data,
            url: addonInfo.url,
          },
        };
      } catch (addonError: unknown) {
        console.error(
          `Failed to fetch addon info for '${node.data.addon}': ` +
          `${(addonError as Error).message}`
        );
        return node;
      }
    })
  );
};

export const enhanceNodesWithCommands = (
  nodes: CustomNodeType[],
  options: {
    nodeSourceCmdMap: Record<string, Set<string>>;
    nodeTargetCmdMap: Record<string, Set<string>>;
    nodeSourceDataMap: Record<string, Set<string>>;
    nodeSourceAudioFrameMap: Record<string, Set<string>>;
    nodeSourceVideoFrameMap: Record<string, Set<string>>;
    nodeTargetDataMap: Record<string, Set<string>>;
    nodeTargetAudioFrameMap: Record<string, Set<string>>;
    nodeTargetVideoFrameMap: Record<string, Set<string>>;
  }
): CustomNodeType[] => {
  const {
    nodeSourceCmdMap = {},
    nodeTargetCmdMap = {},
    nodeSourceDataMap = {},
    nodeSourceAudioFrameMap = {},
    nodeSourceVideoFrameMap = {},
    nodeTargetDataMap = {},
    nodeTargetAudioFrameMap = {},
    nodeTargetVideoFrameMap = {},
  } = options;

  return nodes.map((node) => {
    const sourceCmds = nodeSourceCmdMap[node.id]
      ? Array.from(nodeSourceCmdMap[node.id])
      : [];
    const targetCmds = nodeTargetCmdMap[node.id]
      ? Array.from(nodeTargetCmdMap[node.id])
      : [];

    const sourceData = nodeSourceDataMap[node.id]
      ? Array.from(nodeSourceDataMap[node.id])
      : [];
    const targetData = nodeTargetDataMap[node.id]
      ? Array.from(nodeTargetDataMap[node.id])
      : [];

    const sourceAudioFrame = nodeSourceAudioFrameMap[node.id]
      ? Array.from(nodeSourceAudioFrameMap[node.id])
      : [];
    const targetAudioFrame = nodeTargetAudioFrameMap[node.id]
      ? Array.from(nodeTargetAudioFrameMap[node.id])
      : [];

    const sourceVideoFrame = nodeSourceVideoFrameMap[node.id]
      ? Array.from(nodeSourceVideoFrameMap[node.id])
      : [];
    const targetVideoFrame = nodeTargetVideoFrameMap[node.id]
      ? Array.from(nodeTargetVideoFrameMap[node.id])
      : [];

    return {
      ...node,
      type: "customNode",
      data: {
        ...node.data,
        label: node.data.name || `${node.id}`,
        sourceCmds,
        targetCmds,
        sourceData,
        targetData,
        sourceAudioFrame,
        targetAudioFrame,
        sourceVideoFrame,
        targetVideoFrame,
      },
    };
  });
};
