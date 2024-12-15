//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
import React from "react";
import { FaCogs, FaArrowsAlt } from "react-icons/fa";

import DropdownMenu, { DropdownMenuItem } from "./DropdownMenu";

interface EditMenuProps {
  isOpen: boolean;
  onClick: () => void;
  onHover: () => void;
  onOpenSettings: () => void;
  closeMenu: () => void;
  onAutoLayout: () => void;
}

const EditMenu: React.FC<EditMenuProps> = ({
  isOpen,
  onClick,
  onHover,
  onOpenSettings,
  closeMenu,
  onAutoLayout,
}) => {
  const items: DropdownMenuItem[] = [
    {
      label: "Auto Layout",
      icon: <FaArrowsAlt />,
      onClick: () => {
        onAutoLayout();
        closeMenu();
      },
    },
    {
      label: "Settings",
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
      items={items} // 傳遞 items
    />
  );
};

export default EditMenu;
