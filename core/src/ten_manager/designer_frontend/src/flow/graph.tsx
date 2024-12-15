//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
import { MarkerType } from "@xyflow/react";
import dagre from "dagre";

import { CustomNodeType } from "./CustomNode";
import { CustomEdgeType } from "./CustomEdge";
import { BackendNode, BackendConnection } from "../api/interface";
import { fetchExtensionAddonByName, ExtensionAddon } from "../api/api";

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

export const processNodes = (backendNodes: BackendNode[]): CustomNodeType[] => {
  return backendNodes.map((n, index) => ({
    id: n.name,
    position: { x: index * 200, y: 100 },
    type: "customNode",
    data: {
      name: `${n.name}`,
      addon: n.addon,
      sourceCmds: [],
      targetCmds: [],
    },
  }));
};

export const processConnections = (
  backendConnections: BackendConnection[]
): {
  initialEdges: CustomEdgeType[];
  nodeSourceCmdMap: Record<string, Set<string>>;
  nodeTargetCmdMap: Record<string, Set<string>>;
} => {
  let initialEdges: CustomEdgeType[] = [];
  const nodeSourceCmdMap: Record<string, Set<string>> = {};
  const nodeTargetCmdMap: Record<string, Set<string>> = {};

  backendConnections.forEach((c) => {
    const sourceNodeId = c.extension;
    if (c.cmd) {
      c.cmd.forEach((cmdItem) => {
        cmdItem.dest.forEach((d) => {
          const targetNodeId = d.extension;
          const edgeId =
            `edge-${sourceNodeId}-` + `${cmdItem.name}-${targetNodeId}`;
          const cmdName = cmdItem.name;

          // Record the cmd name of the source node.
          if (!nodeSourceCmdMap[sourceNodeId]) {
            nodeSourceCmdMap[sourceNodeId] = new Set();
          }
          nodeSourceCmdMap[sourceNodeId].add(cmdName);

          // Record the cmd name of the target node.
          if (!nodeTargetCmdMap[targetNodeId]) {
            nodeTargetCmdMap[targetNodeId] = new Set();
          }
          nodeTargetCmdMap[targetNodeId].add(cmdName);

          initialEdges.push({
            id: edgeId,
            source: sourceNodeId,
            target: targetNodeId,
            type: "customEdge",
            label: cmdName,
            sourceHandle: `source-${cmdName}`,
            targetHandle: `target-${cmdName}`,
            markerEnd: {
              type: MarkerType.ArrowClosed,
            },
          });
        });
      });
    }
  });

  return { initialEdges, nodeSourceCmdMap, nodeTargetCmdMap };
};

export const fetchAddonInfoForNodes = async (
  nodes: CustomNodeType[]
): Promise<CustomNodeType[]> => {
  return await Promise.all(
    nodes.map(async (node) => {
      try {
        const addonInfo: ExtensionAddon = await fetchExtensionAddonByName(
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
      } catch (addonError: any) {
        console.error(
          `Failed to fetch addon info for '${node.data.addon}': ` +
            `${addonError.message}`
        );
        return node;
      }
    })
  );
};

export const enhanceNodesWithCommands = (
  nodes: CustomNodeType[],
  nodeSourceCmdMap: Record<string, Set<string>>,
  nodeTargetCmdMap: Record<string, Set<string>>
): CustomNodeType[] => {
  return nodes.map((node) => {
    const sourceCmds = nodeSourceCmdMap[node.id]
      ? Array.from(nodeSourceCmdMap[node.id])
      : [];
    const targetCmds = nodeTargetCmdMap[node.id]
      ? Array.from(nodeTargetCmdMap[node.id])
      : [];
    return {
      ...node,
      type: "customNode",
      data: {
        ...node.data,
        label: node.data.name || `${node.id}`,
        sourceCmds,
        targetCmds,
      },
    };
  });
};
