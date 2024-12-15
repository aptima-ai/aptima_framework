//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
import React from "react";

import "./DropdownMenu.scss";

export interface DropdownMenuItem {
  label?: string;
  icon?: React.ReactNode;
  onClick?: () => void;
  separator?: boolean;
}

interface DropdownMenuProps {
  title: string;
  isOpen: boolean;
  onClick: () => void;
  onHover: () => void;
  items: DropdownMenuItem[];
}

const DropdownMenu: React.FC<DropdownMenuProps> = ({
  title,
  isOpen,
  onClick,
  onHover,
  items,
}) => {
  return (
    <div className="menu-container" onMouseEnter={onHover}>
      {/* Menu title. */}
      <div className="menu-title" onClick={onClick}>
        {title}
      </div>

      {isOpen && (
        <div className="dropdown-menu">
          {items.map((item, index) => (
            <React.Fragment key={index}>
              {/* Separator. */}
              {item.separator && <div className="separator"></div>}
              {/* Menu item. */}
              {!item.separator && (
                <div className="menu-item" onClick={item.onClick}>
                  {/* Icon. Always render the icon, even if no icon. */}
                  <span className="menu-icon">{item.icon || null}</span>{" "}
                  <span className="menu-label">{item.label}</span>
                </div>
              )}
            </React.Fragment>
          ))}
        </div>
      )}
    </div>
  );
};

export default DropdownMenu;
