//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
import React, { useEffect, useState, useCallback } from "react";
import { addEdge, applyEdgeChanges, applyNodeChanges } from "@xyflow/react";

import AppBar from "./components/AppBar/AppBar";
import FlowCanvas from "./flow/FlowCanvas";
import SettingsPopup from "./components/SettingsPopup/SettingsPopup";
import { useTheme } from "./hooks/useTheme";
import {
  fetchConnections,
  fetchDesignerVersion,
  fetchGraphs,
  fetchNodes,
  setBaseDir,
} from "./api/api";
import { Graph } from "./api/interface";
import { CustomNodeType } from "./flow/CustomNode";
import { CustomEdgeType } from "./flow/CustomEdge";
import {
  enhanceNodesWithCommands,
  fetchAddonInfoForNodes,
  getLayoutedElements,
  processConnections,
  processNodes,
} from "./flow/graph";
import Popup from "./components/Popup/Popup";

import "./theme/index.scss";

const App: React.FC = () => {
  const [version, setVersion] = useState<string>("");
  const [showSettings, setShowSettings] = useState(false);
  const [graphs, setGraphs] = useState<Graph[]>([]);
  const [showGraphSelection, setShowGraphSelection] = useState<boolean>(false);
  const [nodes, setNodes] = useState<CustomNodeType[]>([]);
  const [edges, setEdges] = useState<CustomEdgeType[]>([]);
  const [successMessage, setSuccessMessage] = useState<string | null>(null);
  const [errorMessage, setErrorMessage] = useState<string | null>(null);

  const { theme, setTheme } = useTheme();

  // Get the version of tman.
  useEffect(() => {
    fetchDesignerVersion()
      .then((version) => {
        setVersion(version);
      })
      .catch((err) => {
        console.error("Failed to fetch version:", err);
      });
  }, []);

  const handleOpenExistingGraph = useCallback(async () => {
    try {
      const fetchedGraphs = await fetchGraphs();
      setGraphs(fetchedGraphs);
      setShowGraphSelection(true);
    } catch (err: any) {
      console.error(err);
    }
  }, []);

  const handleSelectGraph = useCallback(async (graphName: string) => {
    setShowGraphSelection(false);

    try {
      const backendNodes = await fetchNodes(graphName);
      const backendConnections = await fetchConnections(graphName);

      let initialNodes: CustomNodeType[] = processNodes(backendNodes);

      const { initialEdges, nodeSourceCmdMap, nodeTargetCmdMap } =
        processConnections(backendConnections);

      // Write back the cmd information to nodes, so that CustomNode could
      // generate corresponding handles.
      initialNodes = enhanceNodesWithCommands(
        initialNodes,
        nodeSourceCmdMap,
        nodeTargetCmdMap
      );

      // Fetch additional addon information for each node.
      const nodesWithAddonInfo = await fetchAddonInfoForNodes(initialNodes);

      // Auto-layout the nodes and edges.
      const { nodes: layoutedNodes, edges: layoutedEdges } =
        getLayoutedElements(nodesWithAddonInfo, initialEdges);

      setNodes(layoutedNodes);
      setEdges(layoutedEdges);
    } catch (err: any) {
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

  const handleNodesChange = useCallback((changes: any) => {
    setNodes((nds) => applyNodeChanges(changes, nds));
  }, []);

  const handleEdgesChange = useCallback((changes: any) => {
    setEdges((eds) => applyEdgeChanges(changes, eds));
  }, []);

  const handleSetBaseDir = useCallback(async (folderPath: string) => {
    try {
      await setBaseDir(folderPath);
      setSuccessMessage("Successfully opened a new app folder.");
      setNodes([]); // Clear the contents of the FlowCanvas.
      setEdges([]);
    } catch (error) {
      setErrorMessage("Failed to open a new app folder.");
      console.error(error);
    }
  }, []);

  const handleShowMessage = useCallback(
    (message: string, type: "success" | "error") => {
      if (type === "success") {
        setSuccessMessage(message);
      } else {
        setErrorMessage(message);
      }
    },
    []
  );

  return (
    <div className={`theme-${theme}`}>
      <AppBar
        version={version}
        onOpenSettings={() => setShowSettings(true)}
        onAutoLayout={performAutoLayout}
        onOpenExistingGraph={handleOpenExistingGraph}
        onSetBaseDir={handleSetBaseDir}
        onShowMessage={handleShowMessage}
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
      {showSettings && (
        <SettingsPopup
          theme={theme}
          onChangeTheme={setTheme}
          onClose={() => setShowSettings(false)}
        />
      )}{" "}
      {showGraphSelection && (
        <Popup
          title="Select a Graph"
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
                {graph.name} {graph.auto_start ? "(Auto Start)" : ""}
              </li>
            ))}
          </ul>
        </Popup>
      )}
      {successMessage && (
        <Popup
          title="Ok"
          onClose={() => setSuccessMessage(null)}
          resizable={false}
          initialWidth={300}
          initialHeight={150}
          onCollapseToggle={() => {}}
        >
          <p>{successMessage}</p>
        </Popup>
      )}
      {errorMessage && (
        <Popup
          title="Error"
          onClose={() => setErrorMessage(null)}
          resizable={false}
          initialWidth={300}
          initialHeight={150}
          onCollapseToggle={() => {}}
        >
          <p>{errorMessage}</p>
        </Popup>
      )}
    </div>
  );
};

export default App;
