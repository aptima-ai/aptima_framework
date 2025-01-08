//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
import React, { useState, useCallback } from "react";
import {
  addEdge,
  applyEdgeChanges,
  applyNodeChanges,
  EdgeChange,
  NodeChange,
} from "@xyflow/react";
import { toast } from "sonner";
import { useTranslation } from "react-i18next";

import { ThemeProvider } from "@/components/ThemeProvider";
import AppBar from "@/components/AppBar/AppBar";
import FlowCanvas from "@/flow/FlowCanvas";
import { useVersion } from "@/api/services/common";
import {
  getGraphNodes,
  getGraphConnections,
  getGraphs,
} from "@/api/services/graphs";
import { putBaseDir } from "@/api/services/fileSystem";
import { CustomNodeType } from "@/flow/CustomNode";
import { CustomEdgeType } from "@/flow/CustomEdge";
import {
  enhanceNodesWithCommands,
  fetchAddonInfoForNodes,
  getLayoutedElements,
  processConnections,
  processNodes,
} from "@/flow/graph";
import Popup from "@/components/Popup/Popup";
import type { IGraph } from "@/types/graphs";
import { ReactFlowDataContext } from "@/context/ReactFlowDataContext";

const App: React.FC = () => {
  const [graphs, setGraphs] = useState<IGraph[]>([]);
  const [showGraphSelection, setShowGraphSelection] = useState<boolean>(false);
  const [nodes, setNodes] = useState<CustomNodeType[]>([]);
  const [edges, setEdges] = useState<CustomEdgeType[]>([]);

  // Get the version of tman.
  const { version } = useVersion();
  const { t } = useTranslation();

  const handleOpenExistingGraph = useCallback(async () => {
    try {
      const fetchedGraphs = await getGraphs();
      setGraphs(fetchedGraphs);
      setShowGraphSelection(true);
    } catch (error: unknown) {
      if (error instanceof Error) {
        toast.error(`Failed to fetch graphs: ${error.message}`);
      } else {
        toast.error("An unknown error occurred.");
      }
      console.error(error);
    }
  }, []);

  const handleSelectGraph = useCallback(async (graphName: string) => {
    setShowGraphSelection(false);

    try {
      const backendNodes = await getGraphNodes(graphName);
      const backendConnections = await getGraphConnections(graphName);

      let initialNodes: CustomNodeType[] = processNodes(backendNodes);

      const {
        initialEdges,
        nodeSourceCmdMap,
        nodeSourceDataMap,
        nodeSourceAudioFrameMap,
        nodeSourceVideoFrameMap,
        nodeTargetCmdMap,
        nodeTargetDataMap,
        nodeTargetAudioFrameMap,
        nodeTargetVideoFrameMap,
      } = processConnections(backendConnections);

      // Write back the cmd information to nodes, so that CustomNode could
      // generate corresponding handles.
      initialNodes = enhanceNodesWithCommands(initialNodes, {
        nodeSourceCmdMap,
        nodeTargetCmdMap,
        nodeSourceDataMap,
        nodeTargetDataMap,
        nodeSourceAudioFrameMap,
        nodeSourceVideoFrameMap,
        nodeTargetAudioFrameMap,
        nodeTargetVideoFrameMap,
      });

      // Fetch additional addon information for each node.
      const nodesWithAddonInfo = await fetchAddonInfoForNodes(initialNodes);

      // Auto-layout the nodes and edges.
      const { nodes: layoutedNodes, edges: layoutedEdges } =
        getLayoutedElements(nodesWithAddonInfo, initialEdges);

      setNodes(layoutedNodes);
      setEdges(layoutedEdges);
    } catch (err: unknown) {
      console.error(err);
    }
  }, []);

  const performAutoLayout = useCallback(() => {
    const { nodes: layoutedNodes, edges: layoutedEdges } = getLayoutedElements(
      nodes,
      edges
    );
    setNodes(layoutedNodes);
    setEdges(layoutedEdges);
  }, [nodes, edges]);

  const handleNodesChange = useCallback(
    (changes: NodeChange<CustomNodeType>[]) => {
      setNodes((nds) => applyNodeChanges(changes, nds));
    },
    []
  );

  const handleEdgesChange = useCallback(
    (changes: EdgeChange<CustomEdgeType>[]) => {
      setEdges((eds) => applyEdgeChanges(changes, eds));
    },
    []
  );

  const handleSetBaseDir = useCallback(async (folderPath: string) => {
    try {
      await putBaseDir(folderPath.trim());
      setNodes([]); // Clear the contents of the FlowCanvas.
      setEdges([]);
    } catch (error: unknown) {
      if (error instanceof Error) {
        toast.error(`Failed to open a new app folder`, {
          description: error.message,
        });
      } else {
        toast.error("An unknown error occurred.");
      }
      console.error(error);
    }
  }, []);

  return (
    <ThemeProvider defaultTheme="dark" storageKey="vite-ui-theme">
      <ReactFlowDataContext.Provider value={{ nodes, edges }}>
        <AppBar
          version={version}
          onAutoLayout={performAutoLayout}
          onOpenExistingGraph={handleOpenExistingGraph}
          onSetBaseDir={handleSetBaseDir}
        />
        <FlowCanvas
          nodes={nodes}
          edges={edges}
          onNodesChange={handleNodesChange}
          onEdgesChange={handleEdgesChange}
          onConnect={(connection) => {
            setEdges((eds) => addEdge(connection, eds));
          }}
        />
        {showGraphSelection && (
          <Popup
            title={t("popup.selectGraph.title")}
            onClose={() => setShowGraphSelection(false)}
            resizable={false}
            initialWidth={400}
            initialHeight={300}
            onCollapseToggle={() => {}}
          >
            <ul>
              {graphs.map((graph) => (
                <li
                  key={graph.name}
                  style={{ cursor: "pointer", padding: "5px 0" }}
                  onClick={() => handleSelectGraph(graph.name)}
                >
                  {graph.name}{" "}
                  {graph.auto_start ? `(${t("action.autoStart")})` : ""}
                </li>
              ))}
            </ul>
          </Popup>
        )}
      </ReactFlowDataContext.Provider>
    </ThemeProvider>
  );
};

export default App;
