//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
import React from "react";
import { useTranslation } from "react-i18next";
import { FaEdit, FaTrash, FaTerminal } from "react-icons/fa";

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
      label: t("Edit manifest.json"),
      icon: <FaEdit />,
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
      label: t("Edit property.json"),
      icon: <FaEdit />,
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
      label: t("Launch terminal"),
      icon: <FaTerminal />,
      onClick: () => {
        onClose();
        onLaunchTerminal({ title: node.data.name, url: node.data.url });
      },
    },
    {
      separator: true,
    },
    {
      label: t("Delete"),
      icon: <FaTrash />,
      onClick: () => {
        onClose();
      },
    },
  ];

  return <ContextMenu visible={visible} x={x} y={y} items={items} />;
};

export default NodeContextMenu;
