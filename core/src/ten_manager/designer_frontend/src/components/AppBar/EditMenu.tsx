//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
import React from "react";
import { FaCogs, FaArrowsAlt, FaFolderOpen } from "react-icons/fa";
import { useTranslation } from "react-i18next";

import DropdownMenu, { DropdownMenuItem } from "./DropdownMenu";

interface EditMenuProps {
  isOpen: boolean;
  onClick: () => void;
  onHover: () => void;
  onOpenSettings: () => void;
  closeMenu: () => void;
  onAutoLayout: () => void;
  onOpenExistingGraph: () => void;
}

const EditMenu: React.FC<EditMenuProps> = ({
  isOpen,
  onClick,
  onHover,
  onOpenSettings,
  closeMenu,
  onAutoLayout,
  onOpenExistingGraph,
}) => {
  const { t } = useTranslation();

  const items: DropdownMenuItem[] = [
    {
      label: t("Open Existing Graph"),
      icon: <FaFolderOpen />,
      onClick: () => {
        onOpenExistingGraph();
        closeMenu();
      },
    },
    {
      label: t("Auto Layout"),
      icon: <FaArrowsAlt />,
      onClick: () => {
        onAutoLayout();
        closeMenu();
      },
    },
    {
      label: t("Settings"),
      icon: <FaCogs />,
      onClick: () => {
        onOpenSettings();
        closeMenu();
      },
    },
    {
      separator: true,
    },
  ];

  return (
    <DropdownMenu
      title="Edit"
      isOpen={isOpen}
      onClick={onClick}
      onHover={onHover}
      items={items}
    />
  );
};

export default EditMenu;
