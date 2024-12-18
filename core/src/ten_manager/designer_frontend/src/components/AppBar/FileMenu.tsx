//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
import React, { useState } from "react";
import { FaFolderOpen } from "react-icons/fa";

import DropdownMenu, { DropdownMenuItem } from "./DropdownMenu";
import { setBaseDir } from "../../api/api";
import Popup from "../Popup/Popup";

interface FileMenuProps {
  isOpen: boolean;
  onClick: () => void;
  onHover: () => void;
  closeMenu: () => void;
  onSetBaseDir: (folderPath: string) => void;
  onShowMessage: (message: string, type: "success" | "error") => void;
}

const FileMenu: React.FC<FileMenuProps> = ({
  isOpen,
  onClick,
  onHover,
  closeMenu,
  onSetBaseDir,
  onShowMessage,
}) => {
  const [isFolderPathModalOpen, setIsFolderPathModalOpen] =
    useState<boolean>(false);
  const [folderPath, setFolderPath] = useState<string>("");

  const handleManualOk = async () => {
    if (!folderPath.trim()) {
      onShowMessage("The folder path cannot be empty.", "error");
      return;
    }

    try {
      await setBaseDir(folderPath.trim());
      onSetBaseDir(folderPath.trim());
      setFolderPath("");
    } catch (error) {
      onShowMessage("Failed to open a new app folder.", "error");
      console.error(error);
    }
  };

  const items: DropdownMenuItem[] = [
    {
      label: "Open App Folder",
      icon: <FaFolderOpen />,
      onClick: () => {
        setIsFolderPathModalOpen(true);
        closeMenu();
      },
    },
  ];

  return (
    <>
      <DropdownMenu
        title="File"
        isOpen={isOpen}
        onClick={onClick}
        onHover={onHover}
        items={items}
      />
      {isFolderPathModalOpen && (
        <Popup
          title="Open App Folder"
          onClose={() => setIsFolderPathModalOpen(false)}
          resizable={false}
          initialWidth={400}
          initialHeight={200}
          onCollapseToggle={() => {}}
        >
          <div>
            <label htmlFor="folderPath">Folder Path:</label>
            <input
              type="text"
              id="folderPath"
              value={folderPath}
              onChange={(e) => setFolderPath(e.target.value)}
              style={{ width: "100%", padding: "8px", marginTop: "5px" }}
              placeholder="Enter folder path"
            />
            <button
              onClick={handleManualOk}
              style={{ marginTop: "10px", padding: "8px 16px" }}
            >
              Ok
            </button>
          </div>
        </Popup>
      )}
    </>
  );
};

export default FileMenu;
