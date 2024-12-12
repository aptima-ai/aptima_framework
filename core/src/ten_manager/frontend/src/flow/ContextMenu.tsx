//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
import React from "react";
import "./ContextMenu.scss";
import { useTranslation } from "react-i18next";

interface ContextMenuProps {
  visible: boolean;
  x: number;
  y: number;
  type?: "node" | "edge";
  data?: any;
  onClose: () => void;
}

const ContextMenu: React.FC<ContextMenuProps> = ({
  visible,
  x,
  y,
  type,
  data,
  onClose,
}) => {
  const { t } = useTranslation();

  if (!visible) return null;

  const renderMenuItems = () => {
    if (type === "node" && data) {
      return (
        <>
          <div
            onClick={() => {
              onClose();
            }}
          >
            {t("Edit")}
          </div>
          <div
            onClick={() => {
              onClose();
            }}
          >
            {t("Delete")}
          </div>
        </>
      );
    } else if (type === "edge" && data) {
      return (
        <>
          <div
            onClick={() => {
              onClose();
            }}
          >
            {t("Edit")}
          </div>
          <div
            onClick={() => {
              onClose();
            }}
          >
            {t("Delete")}
          </div>
        </>
      );
    } else {
      return (
        <>
          <div onClick={onClose}>Close Menu</div>
        </>
      );
    }
  };

  return (
    <div
      className="context-menu"
      style={{
        left: x,
        top: y,
      }}
      onClick={(e) => e.stopPropagation()}
    >
      {renderMenuItems()}
    </div>
  );
};

export default ContextMenu;
