//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
import React from "react";
import { FaFolderOpen } from "react-icons/fa";

import DropdownMenu, { DropdownMenuItem } from "./DropdownMenu";

interface FileMenuProps {
  isOpen: boolean;
  onClick: () => void;
  onHover: () => void;
  closeMenu: () => void;
}

const FileMenu: React.FC<FileMenuProps> = ({
  isOpen,
  onClick,
  onHover,
  closeMenu,
}) => {
  const items: DropdownMenuItem[] = [
    {
      label: "Open TEN app folder",
      icon: <FaFolderOpen />,
      onClick: () => {
        closeMenu();
      },
    },
  ];

  return (
    <DropdownMenu
      title="File"
      isOpen={isOpen}
      onClick={onClick}
      onHover={onHover}
      items={items}
    />
  );
};

export default FileMenu;
