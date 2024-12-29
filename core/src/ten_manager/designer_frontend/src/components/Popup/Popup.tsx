//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
import React, { useState, useEffect, useRef } from "react";
import { ChevronDown, ChevronUp, X } from "lucide-react";

import { cn } from "@/lib/utils";
import { Button } from "@/components/ui/Button";

const DEFAULT_WIDTH = 800;
const DEFAULT_HEIGHT = 400;

interface PopupProps {
  title: string;
  children: React.ReactNode;
  className?: string;
  resizable?: boolean;

  // When a popup appears, it takes keyboard focus, allowing the popup to be
  // closed directly by pressing "Esc." However, in some scenarios, the popup
  // should not take keyboard focus. For example, in the case of xterm, if the
  // popup takes keyboard focus, xterm will continuously fail to regain keyboard
  //  focus, making it impossible to receive input from the keyboard.
  preventFocusSteal?: boolean;

  initialWidth?: number;
  initialHeight?: number;

  onClose: () => void;
  onCollapseToggle?: (isCollapsed: boolean) => void;
}

const Popup: React.FC<PopupProps> = ({
  title,
  children,
  className,
  resizable = false, // Default to non-resizable.
  preventFocusSteal = false,
  initialWidth,
  initialHeight,
  onClose,
  onCollapseToggle,
}) => {
  const popupRef = useRef<HTMLDivElement>(null);
  const headerRef = useRef<HTMLDivElement>(null);

  const [isCollapsed, setIsCollapsed] = useState(false);
  const [isVisible, setIsVisible] = useState(false);

  const [position, setPosition] = useState({ x: 0, y: 0 });
  const [size, setSize] = useState<{ width?: number; height?: number }>({
    width: initialWidth,
    height: initialHeight,
  });
  const [prevHeight, setPrevHeight] = useState<number | null>(null);
  const [headerHeight, setHeaderHeight] = useState<number>(0);
  const [isResizing, setIsResizing] = useState(false);
  const [resizeStart, setResizeStart] = useState<{
    x: number;
    y: number;
    width: number;
    height: number;
  } | null>(null);

  const [isDragging, setIsDragging] = useState(false);
  const [dragStart, setDragStart] = useState<{ x: number; y: number } | null>(
    null
  );

  // Center the popup on mount.
  useEffect(() => {
    if (popupRef.current) {
      const { innerWidth, innerHeight } = window;
      const rect = popupRef.current.getBoundingClientRect();
      setPosition({
        x: (innerWidth - rect.width) / 2,
        y: (innerHeight - rect.height) / 2,
      });
      if (!preventFocusSteal) {
        popupRef.current.focus();
      }
    }
    setIsVisible(true);
  }, [preventFocusSteal]);

  // If an initial size is not provided, the actual size will be determined
  // after the popup becomes visible.
  useEffect(() => {
    if (
      isVisible &&
      popupRef.current &&
      size.width === undefined &&
      size.height === undefined
    ) {
      const rect = popupRef.current.getBoundingClientRect();
      setSize({ width: rect.width, height: rect.height });
    }
  }, [isVisible, size.width, size.height]);

  // Handle keydown for ESC.
  useEffect(() => {
    const handleKeyDown = (e: KeyboardEvent) => {
      if (e.key === "Escape") {
        onClose();
      }
    };

    // Only add listener if this popup should take focus.
    if (!preventFocusSteal && popupRef.current) {
      const current = popupRef.current;
      current.addEventListener("keydown", handleKeyDown);

      // Cleanup function uses the captured reference.
      return () => {
        current.removeEventListener("keydown", handleKeyDown);
      };
    }

    // If the condition isn't met, no cleanup is necessary.
    return undefined;
  }, [onClose, preventFocusSteal]);

  const handleMouseDown = (e: React.MouseEvent) => {
    // Bring to front immediately on mouse down.
    bringToFront();

    setIsDragging(true);
    setDragStart({ x: e.clientX - position.x, y: e.clientY - position.y });
  };

  useEffect(() => {
    const handleMouseMove = (e: MouseEvent) => {
      if (isDragging && dragStart) {
        setPosition({ x: e.clientX - dragStart.x, y: e.clientY - dragStart.y });
      }

      if (isResizing && resizeStart) {
        const newWidth = Math.max(e.clientX - resizeStart.x, 300);
        const newHeight = Math.max(e.clientY - resizeStart.y, 200);
        setSize({
          width: newWidth,
          height: newHeight,
        });
      }
    };

    const handleMouseUp = () => {
      setIsDragging(false);
      setDragStart(null);

      setIsResizing(false);
      setResizeStart(null);
    };

    if (isDragging || isResizing) {
      window.addEventListener("mousemove", handleMouseMove);
      window.addEventListener("mouseup", handleMouseUp);
    } else {
      window.removeEventListener("mousemove", handleMouseMove);
      window.removeEventListener("mouseup", handleMouseUp);
    }

    return () => {
      window.removeEventListener("mousemove", handleMouseMove);
      window.removeEventListener("mouseup", handleMouseUp);
    };
  }, [isDragging, isResizing, dragStart, resizeStart]);

  const handleResizeMouseDown = (e: React.MouseEvent) => {
    e.stopPropagation(); // Prevent from triggering dragging.
    setIsResizing(true);
    setResizeStart({
      x: position.x,
      y: position.y,
      width: size.width ?? DEFAULT_WIDTH,
      height: size.height ?? DEFAULT_HEIGHT,
    });
  };

  useEffect(() => {
    if (headerRef.current) {
      const height = headerRef.current.getBoundingClientRect().height;
      setHeaderHeight(height);
    }
    bringToFront();
  }, []);

  const toggleCollapse = () => {
    setIsCollapsed((prev) => {
      const newState = !prev;
      if (newState) {
        // Store the current height before collapsing.
        setPrevHeight(size.height ?? null);

        // Set the height to the header's height to collapse.
        setSize((prevSize) => ({ ...prevSize, height: headerHeight }));
      } else if (prevHeight !== null) {
        // Restore the previous height when un-collapsing.
        setSize((prevSize) => ({ ...prevSize, height: prevHeight }));

        setPrevHeight(null);
      }
      onCollapseToggle?.(newState);
      return newState;
    });
  };

  // Handle focus and bring to front when clicked.
  const handleClick = () => {
    if (!preventFocusSteal) {
      popupRef.current?.focus();
      bringToFront();
    }
  };

  const bringToFront = () => {
    const highestZIndex = Math.max(
      ...Array.from(document.querySelectorAll(".popup")).map(
        (el) => parseInt(window.getComputedStyle(el).zIndex) || 0
      )
    );
    if (popupRef.current) {
      popupRef.current.style.zIndex = (highestZIndex + 1).toString();
    }
  };

  return (
    <div
      className={cn(
        "fixed bg-background border border-border shadow-lg text-sm",
        "text-foreground rounded focus:outline-none flex flex-col popup",
        isVisible ? "opacity-100" : "opacity-0",
        isResizing && "select-none",
        isDragging && "select-none cursor-move",
        className
      )}
      style={{
        top: position.y,
        left: position.x,
        zIndex: 1001,
        ...(size.width !== undefined && size.height !== undefined
          ? { width: size.width, height: size.height }
          : {}),
      }}
      ref={popupRef}
      tabIndex={-1}
      onClick={handleClick}
      onMouseDownCapture={() => {
        if (!preventFocusSteal) {
          popupRef.current?.focus();
        }
        bringToFront();
      }}
    >
      <div
        className={cn(
          "p-2.5 flex justify-between items-center cursor-move select-none",
          "bg-muted border-b border-border"
        )}
        onMouseDown={handleMouseDown}
        ref={headerRef}
      >
        <span className="font-bold">{title}</span>
        <div className="flex items-center gap-1.5">
          <Button
            variant="ghost"
            size="sm"
            className="h-auto p-1.5"
            onClick={toggleCollapse}
          >
            {isCollapsed ? <ChevronDown /> : <ChevronUp />}
          </Button>
          <Button
            variant="ghost"
            size="sm"
            className="h-auto p-1.5"
            onClick={onClose}
          >
            <X />
          </Button>
        </div>
      </div>

      <div
        className={cn(
          "p-2.5 overflow-hidden flex w-full h-full",
          isCollapsed ? "invisible h-0 py-0" : "visible"
        )}
      >
        {children}
      </div>

      {resizable && !isCollapsed && (
        <div
          className={cn(
            "w-[5px] h-[5px] bg-transparent cursor-se-resize",
            "absolute right-0 bottom-0 z-[1002]"
          )}
          onMouseDown={handleResizeMouseDown}
        />
      )}
    </div>
  );
};

export default Popup;
