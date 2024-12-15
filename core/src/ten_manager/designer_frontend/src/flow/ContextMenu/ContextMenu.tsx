//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
import React from "react";

import "./ContextMenu.scss";

export interface ContextMenuItem {
  label?: string;
  icon?: React.ReactNode;
  onClick?: () => void;
  separator?: boolean;
}

interface ContextMenuProps {
  visible: boolean;
  x: number;
  y: number;
  items: ContextMenuItem[];
}

const ContextMenu: React.FC<ContextMenuProps> = ({ visible, x, y, items }) => {
  if (!visible) return null;

  return (
    <div
      className="context-menu"
      style={{
        left: x,
        top: y,
      }}
      onClick={(e) => e.stopPropagation()}
    >
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
  );
};

export default ContextMenu;
