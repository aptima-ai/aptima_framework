//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
import React from "react";

import { cn } from "@/lib/utils";

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
      className={cn(
        "fixed p-1.5 z-[9999]",
        "bg-white border border-gray-300 shadow-lg box-border"
      )}
      style={{
        left: x,
        top: y,
      }}
      onClick={(e) => e.stopPropagation()}
    >
      {items.map((item, index) => (
        <React.Fragment key={index}>
          {item.separator && <div className="h-px bg-gray-300 my-1.5"></div>}
          {!item.separator && (
            <div
              className={cn(
                "flex items-center px-2.5 py-1.5 whitespace-nowrap",
                "box-border hover:bg-gray-100 cursor-pointer"
              )}
              onClick={item.onClick}
            >
              <span
                className={cn(
                  "flex items-center flex-shrink-0 justify-center",
                  "mr-2 h-[1em] w-5"
                )}
              >
                {item.icon || null}
              </span>
              <span className="flex-1 text-left">{item.label}</span>
            </div>
          )}
        </React.Fragment>
      ))}
    </div>
  );
};

export default ContextMenu;
