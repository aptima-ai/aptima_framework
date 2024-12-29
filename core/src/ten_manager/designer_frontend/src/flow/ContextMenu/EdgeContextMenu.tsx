//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
import React from "react";
import { useTranslation } from "react-i18next";
import { FaEdit, FaTrash } from "react-icons/fa";

import ContextMenu, { ContextMenuItem } from "./ContextMenu";
import { CustomEdgeType } from "../CustomEdge";

interface EdgeContextMenuProps {
  visible: boolean;
  x: number;
  y: number;
  edge: CustomEdgeType;
  onClose: () => void;
}

const EdgeContextMenu: React.FC<EdgeContextMenuProps> = ({
  visible,
  x,
  y,
  onClose,
}) => {
  const { t } = useTranslation();

  const items: ContextMenuItem[] = [
    {
      label: t("Edit"),
      icon: <FaEdit />,
      onClick: () => {
        onClose();
      },
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

export default EdgeContextMenu;
