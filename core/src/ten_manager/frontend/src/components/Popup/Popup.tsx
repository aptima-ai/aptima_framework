//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
import React, { useState, useEffect, useRef } from "react";
import "./Popup.scss";

interface PopupProps {
  title: string;
  onClose: () => void;
  children: React.ReactNode;
}

const Popup: React.FC<PopupProps> = ({ title, onClose, children }) => {
  const [isCollapsed, setIsCollapsed] = useState(false);
  const [position, setPosition] = useState({ x: 0, y: 0 });
  const [dragging, setDragging] = useState(false);
  const [dragStart, setDragStart] = useState<{ x: number; y: number } | null>(
    null
  );
  const [isVisible, setIsVisible] = useState(false);
  const popupRef = useRef<HTMLDivElement>(null);

  // Center the popup on mount.
  useEffect(() => {
    if (popupRef.current) {
      const { innerWidth, innerHeight } = window;
      const rect = popupRef.current.getBoundingClientRect();
      setPosition({
        x: (innerWidth - rect.width) / 2,
        y: (innerHeight - rect.height) / 2,
      });
      popupRef.current.focus();
    }
    setIsVisible(true);
  }, []);

  // Handle keydown for ESC.
  useEffect(() => {
    const handleKeyDown = (e: KeyboardEvent) => {
      if (e.key === "Escape") {
        onClose();
      }
    };

    // Only add listener if this popup is focused.
    const currentPopup = popupRef.current;
    if (currentPopup) {
      currentPopup.addEventListener("keydown", handleKeyDown);
    }

    return () => {
      if (currentPopup) {
        currentPopup.removeEventListener("keydown", handleKeyDown);
      }
    };
  }, [onClose]);

  const handleMouseDown = (e: React.MouseEvent) => {
    setDragging(true);
    setDragStart({ x: e.clientX - position.x, y: e.clientY - position.y });
  };

  const handleMouseMove = (e: MouseEvent) => {
    if (dragging && dragStart) {
      setPosition({ x: e.clientX - dragStart.x, y: e.clientY - dragStart.y });
    }
  };

  const handleMouseUp = () => {
    setDragging(false);
    setDragStart(null);
  };

  useEffect(() => {
    if (dragging) {
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
  }, [dragging, dragStart]);

  const toggleCollapse = () => {
    setIsCollapsed((prev) => !prev);
  };

  // Handle focus when clicked.
  const handleClick = () => {
    popupRef.current?.focus();
    bringToFront();
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
      className={`popup ${isVisible ? "visible" : "hidden"}`}
      style={{
        top: position.y,
        left: position.x,
      }}
      ref={popupRef}
      tabIndex={-1}
      onClick={handleClick}
    >
      <div className="popup-header" onMouseDown={handleMouseDown}>
        <span className="popup-title">{title}</span>
        <div className="popup-buttons">
          <button className="collapse-button" onClick={toggleCollapse}>
            {isCollapsed ? "▼" : "▲"}
          </button>
          <button className="close-button" onClick={onClose}>
            ×
          </button>
        </div>
      </div>
      {!isCollapsed && <div className="popup-content">{children}</div>}
    </div>
  );
};

export default Popup;
