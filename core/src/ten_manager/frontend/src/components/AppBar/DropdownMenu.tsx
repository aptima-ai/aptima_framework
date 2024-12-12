//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
import React from "react";
import "./DropdownMenu.scss";

interface DropdownMenuProps {
  title: string;
  isOpen: boolean;
  onClick: () => void;
  onHover: () => void;
  children: React.ReactNode;
}

const DropdownMenu: React.FC<DropdownMenuProps> = ({
  title,
  isOpen,
  onClick,
  onHover,
  children,
}) => {
  return (
    <div className="menu-container" onMouseEnter={onHover}>
      <div className="menu-title" onClick={onClick}>
        {title}
      </div>
      {isOpen && <div className="dropdown-menu">{children}</div>}
    </div>
  );
};

export default DropdownMenu;
