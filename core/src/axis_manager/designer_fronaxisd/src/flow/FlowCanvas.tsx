//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
import {
  useEffect,
  useState,
  useCallback,
  forwardRef,
  MouseEvent,
  useContext,
} from "react";
import {
  ReactFlow,
  MiniMap,
  Controls,
  Connection,
  type NodeChange,
  type EdgeChange,
} from "@xyflow/react";

import CustomNode, { CustomNodeType } from "@/flow/CustomNode";
import CustomEdge, { CustomEdgeType } from "@/flow/CustomEdge";
import NodeContextMenu from "@/flow/ContextMenu/NodeContextMenu";
import EdgeContextMenu from "@/flow/ContextMenu/EdgeContextMenu";
import TerminalPopup, { TerminalData } from "@/components/Popup/TerminalPopup";
import EditorPopup, { EditorData } from "@/components/Popup/EditorPopup";
import CustomNodeConnPopup from "@/components/Popup/CustomNodeConnPopup";
import { ThemeProviderContext } from "@/components/theme-context";

// Import react-flow style.
import "@xyflow/react/dist/style.css";
import "@/flow/reactflow.css";

export interface FlowCanvasRef {
  performAutoLayout: () => void;
}

interface FlowCanvasProps {
  nodes: CustomNodeType[];
  edges: CustomEdgeType[];
  onNodesChange: (changes: NodeChange<CustomNodeType>[]) => void;
  onEdgesChange: (changes: EdgeChange<CustomEdgeType>[]) => void;
  onConnect: (connection: Connection) => void;
}

const FlowCanvas = forwardRef<FlowCanvasRef, FlowCanvasProps>(
  ({ nodes, edges, onNodesChange, onEdgesChange, onConnect }) => {
    const [terminalPopups, setTerminalPopups] = useState<
      { id: string; data: TerminalData }[]
    >([]);
    const [editorPopups, setEditorPopups] = useState<
      { id: string; data: EditorData }[]
    >([]);

    const [contextMenu, setContextMenu] = useState<{
      visible: boolean;
      x: number;
      y: number;
      type?: "node" | "edge";
      edge?: CustomEdgeType;
      node?: CustomNodeType;
    }>({ visible: false, x: 0, y: 0 });
    const [connPopups, setConnPopups] = useState<
      {
        id: string;
        source: string;
        target?: string;
      }[]
    >([]);

    const launchTerminal = (data: TerminalData) => {
      const newPopup = { id: `${data.title}-${Date.now()}`, data };
      setTerminalPopups((prev) => [...prev, newPopup]);
    };

    const closeTerminal = (id: string) => {
      setTerminalPopups((prev) => prev.filter((popup) => popup.id !== id));
    };

    const launchEditor = (data: EditorData) => {
      setEditorPopups((prev) => {
        const existingPopup = prev.find((popup) => popup.data.url === data.url);
        if (existingPopup) {
          return prev;
        } else {
          return [
            ...prev,
            {
              id: `${data.url}-${Date.now()}`,
              data: {
                title: data.title,
                url: data.url,

                // Initializes content to empty, the content will be fetched by
                // EditorPopup.
                content: "",
              },
            },
          ];
        }
      });
    };

    const closeEditor = (id: string) => {
      setEditorPopups((prev) => prev.filter((popup) => popup.id !== id));
    };

    const launchConnPopup = (source: string, target?: string) => {
      setConnPopups((prev) => {
        const existingPopup = prev.find(
          (popup) => popup.source === source && popup.target === target
        );
        if (existingPopup) {
          return prev;
        }
        return [...prev, { source, target, id: `${source}-${target ?? ""}` }];
      });
    };

    const closeConnPopup = (id: string) => {
      setConnPopups((prev) => prev.filter((popup) => popup.id !== id));
    };

    const renderContextMenu = () => {
      if (contextMenu.type === "node" && contextMenu.node) {
        return (
          <NodeContextMenu
            visible={contextMenu.visible}
            x={contextMenu.x}
            y={contextMenu.y}
            node={contextMenu.node}
            onClose={closeContextMenu}
            onLaunchTerminal={launchTerminal}
            onLaunchEditor={launchEditor}
          />
        );
      } else if (contextMenu.type === "edge" && contextMenu.edge) {
        return (
          <EdgeContextMenu
            visible={contextMenu.visible}
            x={contextMenu.x}
            y={contextMenu.y}
            edge={contextMenu.edge}
            onClose={closeContextMenu}
          />
        );
      }
      return null;
    };

    // Right click nodes.
    const clickNodeContextMenu = useCallback(
      (event: MouseEvent, node: CustomNodeType) => {
        event.preventDefault();
        setContextMenu({
          visible: true,
          x: event.clientX,
          y: event.clientY,
          type: "node",
          node: node,
        });
      },
      []
    );

    // Right click Edges.
    const clickEdgeContextMenu = useCallback(
      (event: MouseEvent, edge: CustomEdgeType) => {
        event.preventDefault();
        setContextMenu({
          visible: true,
          x: event.clientX,
          y: event.clientY,
          type: "edge",
          edge: edge,
        });
      },
      []
    );

    // Close context menu.
    const closeContextMenu = useCallback(() => {
      setContextMenu({ visible: false, x: 0, y: 0 });
    }, []);

    // Click empty space to close context menu.
    useEffect(() => {
      const handleClick = () => {
        closeContextMenu();
      };
      const handleCustomNodeAction = (event: CustomEvent) => {
        switch (event.detail.action) {
          case "connections":
            launchConnPopup(event.detail.source, event.detail.target);
            break;
          default:
            break;
        }
      };
      window.addEventListener("click", handleClick);
      window.addEventListener(
        "customNodeAction",
        handleCustomNodeAction as EventListener
      );
      return () => {
        window.removeEventListener("click", handleClick);
        window.removeEventListener(
          "customNodeAction",
          handleCustomNodeAction as EventListener
        );
      };
    }, [closeContextMenu]);

    const { theme } = useContext(ThemeProviderContext);

    return (
      <div
        className="flow-container"
        style={{ width: "100%", height: "calc(100vh - 40px)" }}
      >
        <ReactFlow
          colorMode={theme}
          nodes={nodes}
          edges={edges}
          edgeTypes={{
            customEdge: CustomEdge,
          }}
          nodeTypes={{
            customNode: CustomNode,
          }}
          onNodesChange={onNodesChange}
          onEdgesChange={onEdgesChange}
          onConnect={(p) => onConnect(p)}
          fitView
          nodesDraggable={true}
          edgesFocusable={true}
          style={{ width: "100%", height: "100%" }}
          onNodeContextMenu={clickNodeContextMenu}
          onEdgeContextMenu={clickEdgeContextMenu}
          // onEdgeClick={(e, edge) => {
          //   console.log("clicked", e, edge);
          // }}
        >
          <Controls />
          <MiniMap zoomable pannable />
          <svg className="">
            <defs>
              <linearGradient id="edge-gradient">
                <stop offset="0%" stopColor="#ae53ba" />
                <stop offset="100%" stopColor="#2a8af6" />
              </linearGradient>

              <marker
                id="edge-circle"
                viewBox="-5 -5 10 10"
                refX="0"
                refY="0"
                markerUnits="strokeWidth"
                markerWidth="10"
                markerHeight="10"
                orient="auto"
              >
                <circle
                  stroke="#2a8af6"
                  strokeOpacity="0.75"
                  r="2"
                  cx="0"
                  cy="0"
                />
              </marker>
            </defs>
          </svg>
        </ReactFlow>

        {renderContextMenu()}

        {terminalPopups.map((popup) => (
          <TerminalPopup
            key={popup.id}
            data={popup.data}
            onClose={() => closeTerminal(popup.id)}
          />
        ))}

        {editorPopups.map((popup) => (
          <EditorPopup
            key={popup.id}
            data={popup.data}
            onClose={() => closeEditor(popup.id)}
          />
        ))}

        {connPopups.map((popup) => (
          <CustomNodeConnPopup
            key={popup.id}
            source={popup.source}
            target={popup.target}
            onClose={() => closeConnPopup(popup.id)}
          />
        ))}
      </div>
    );
  }
);

export default FlowCanvas;
