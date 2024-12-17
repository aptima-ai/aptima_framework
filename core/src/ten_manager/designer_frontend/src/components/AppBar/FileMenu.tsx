//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
import React, { useRef, useState } from "react";
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
}

const FileMenu: React.FC<FileMenuProps> = ({
  isOpen,
  onClick,
  onHover,
  closeMenu,
  onSetBaseDir,
}) => {
  const fileInputRef = useRef<HTMLInputElement>(null);
  const [isManualInput, setIsManualInput] = useState<boolean>(false);
  const [manualPath, setManualPath] = useState<string>("");
  const [showPopup, setShowPopup] = useState<boolean>(false);
  const [popupMessage, setPopupMessage] = useState<string>("");

  // Try to use `showDirectoryPicker` in the File System Access API.
  const handleOpenFolder = async () => {
    if ("showDirectoryPicker" in window) {
      try {
        const dirHandle = await (window as any).showDirectoryPicker();
        const folderPath = dirHandle.name;
        onSetBaseDir(folderPath);
      } catch (error) {
        console.error("Directory selection canceled or failed:", error);
      }
    } else {
      // Fallback to input[type="file"].
      if (fileInputRef.current) {
        fileInputRef.current.click();
      }
    }
  };

  const handleFolderSelected = (event: React.ChangeEvent<HTMLInputElement>) => {
    const files = event.target.files;
    if (files && files.length > 0) {
      const firstFile = files[0];
      const relativePath = firstFile.webkitRelativePath;
      const folderPath = relativePath.split("/")[0];
      onSetBaseDir(folderPath);
    }
  };

  const handleManualSubmit = async () => {
    if (!manualPath.trim()) {
      setPopupMessage("The folder path cannot be empty.");
      setShowPopup(true);
      return;
    }

    try {
      await setBaseDir(manualPath.trim());
      setPopupMessage("Successfully opened a new app folder.");
      onSetBaseDir(manualPath.trim());
      setManualPath("");
    } catch (error) {
      setPopupMessage("Failed to open a new app folder.");
      console.error(error);
    } finally {
      setShowPopup(true);
    }
  };

  const items: DropdownMenuItem[] = [
    {
      label: "Open TEN app folder",
      icon: <FaFolderOpen />,
      onClick: () => {
        handleOpenFolder();
        closeMenu();
      },
    },
    {
      label: "Set App Folder Manually",
      icon: <FaFolderOpen />,
      onClick: () => {
        setIsManualInput(true);
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
      {/* Hidden file input used for selecting folders. */}
      <input
        type="file"
        ref={fileInputRef}
        style={{ display: "none" }}
        onChange={handleFolderSelected}
      />

      {/* Popup for manually entering the folder path. */}
      {isManualInput && (
        <Popup
          title="Set App Folder Manually"
          onClose={() => setIsManualInput(false)}
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
              value={manualPath}
              onChange={(e) => setManualPath(e.target.value)}
              style={{ width: "100%", padding: "8px", marginTop: "5px" }}
              placeholder="Enter folder path"
            />
            <button
              onClick={handleManualSubmit}
              style={{ marginTop: "10px", padding: "8px 16px" }}
            >
              Submit
            </button>
          </div>
        </Popup>
      )}

      {/* Popup to display success or error messages. */}
      {showPopup && (
        <Popup
          title={popupMessage.includes("Ok") ? "Ok" : "Error"}
          onClose={() => setShowPopup(false)}
          resizable={false}
          initialWidth={300}
          initialHeight={150}
          onCollapseToggle={() => {}}
        >
          <p>{popupMessage}</p>
        </Popup>
      )}
      {/* >>>> END NEW */}
    </>
  );
};

export default FileMenu;
