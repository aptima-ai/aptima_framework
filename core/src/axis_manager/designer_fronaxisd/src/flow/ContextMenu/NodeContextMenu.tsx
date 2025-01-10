//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
import React from "react";
import { useTranslation } from "react-i18next";
import { FilePenLineIcon, TerminalIcon, Trash2Icon } from "lucide-react";

import ContextMenu, { ContextMenuItem } from "@/flow/ContextMenu/ContextMenu";
import { CustomNodeType } from "@/flow/CustomNode";
import { TerminalData } from "@/components/Popup/TerminalPopup";
import { EditorData } from "@/components/Popup/EditorPopup";

interface NodeContextMenuProps {
  visible: boolean;
  x: number;
  y: number;
  node: CustomNodeType;
  onClose: () => void;
  onLaunchTerminal: (data: TerminalData) => void;
  onLaunchEditor: (data: EditorData) => void;
}

const NodeContextMenu: React.FC<NodeContextMenuProps> = ({
  visible,
  x,
  y,
  node,
  onClose,
  onLaunchTerminal,
  onLaunchEditor,
}) => {
  const { t } = useTranslation();

  const items: ContextMenuItem[] = [
    {
      label: t("action.edit") + " manifest.json",
      icon: <FilePenLineIcon />,
      onClick: () => {
        onClose();
        if (node?.data.url)
          onLaunchEditor({
            title: `${node.data.addon} manifest.json`,
            content: "",
            url: `${node.data.url}/manifest.json`,
          });
      },
    },
    {
      label: t("action.edit") + " property.json",
      icon: <FilePenLineIcon />,
      onClick: () => {
        onClose();
        if (node?.data.url)
          onLaunchEditor({
            title: `${node.data.addon} property.json`,
            content: "",
            url: `${node.data.url}/property.json`,
          });
      },
    },
    {
      separator: true,
    },
    {
      label: t("action.launchTerminal"),
      icon: <TerminalIcon />,
      onClick: () => {
        onClose();
        onLaunchTerminal({ title: node.data.name, url: node.data.url });
      },
    },
    {
      separator: true,
    },
    {
      label: t("action.delete"),
      icon: <Trash2Icon />,
      onClick: () => {
        onClose();
      },
    },
  ];

  return <ContextMenu visible={visible} x={x} y={y} items={items} />;
};

export default NodeContextMenu;
