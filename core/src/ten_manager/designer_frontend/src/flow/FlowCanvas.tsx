//
// Copyright © 2024 Agora
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
} from "react";
import { ReactFlow, MiniMap, Controls, Connection } from "@xyflow/react";

import CustomNode, { CustomNodeType } from "./CustomNode";
import CustomEdge, { CustomEdgeType } from "./CustomEdge";
import NodeContextMenu from "./ContextMenu/NodeContextMenu";
import EdgeContextMenu from "./ContextMenu/EdgeContextMenu";
import TerminalPopup, {
  TerminalData,
} from "../components/TerminalPopup/TerminalPopup";
import EditorPopup, { EditorData } from "../components/EditorPopup/EditorPopup";

export interface FlowCanvasRef {
  performAutoLayout: () => void;
}

interface FlowCanvasProps {
  nodes: CustomNodeType[];
  edges: CustomEdgeType[];
  onNodesChange: (changes: any) => void;
  onEdgesChange: (changes: any) => void;
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
      window.addEventListener("click", handleClick);
      return () => window.removeEventListener("click", handleClick);
    }, [closeContextMenu]);

    return (
      <div
        className="flow-container"
        style={{ width: "100%", height: "calc(100vh - 40px)" }}
      >
        <ReactFlow
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
        >
          <Controls />
          <MiniMap />
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
      </div>
    );
  }
);

export default FlowCanvas;
