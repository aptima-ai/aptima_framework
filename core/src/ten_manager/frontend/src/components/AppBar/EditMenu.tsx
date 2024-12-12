//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
import React from "react";
import DropdownMenu from "./DropdownMenu";

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
  return (
    <DropdownMenu
      title="Edit"
      isOpen={isOpen}
      onClick={onClick}
      onHover={onHover}
    >
      {" "}
      <div
        className="menu-item"
        onClick={() => {
          onAutoLayout();
          closeMenu();
        }}
      >
        Auto Layout
      </div>
      <div
        className="menu-item"
        onClick={() => {
          onOpenSettings();
          closeMenu();
        }}
      >
        Settings
      </div>
    </DropdownMenu>
  );
};

export default EditMenu;
