//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
import React from "react";

import { Button } from "@/components/ui/Button";
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
        "bg-popover border border-border rounded-md shadow-md box-border"
      )}
      style={{
        left: x,
        top: y,
      }}
      onClick={(e) => e.stopPropagation()}
    >
      {items.map((item, index) => (
        <React.Fragment key={index}>
          {item.separator && <div className="h-px bg-border my-1"></div>}
          {!item.separator && (
            <Button
              variant="ghost"
              asChild
              className={cn(
                "flex w-full justify-start px-2.5 py-1.5 whitespace-nowrap",
                "h-auto font-normal cursor-pointer"
              )}
              onClick={item.onClick}
            >
              <div>
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
            </Button>
          )}
        </React.Fragment>
      ))}
    </div>
  );
};

export default ContextMenu;
