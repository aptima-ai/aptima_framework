//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
import React, { useEffect, useState } from "react";
import Editor from "@monaco-editor/react";

import Popup from "../Popup/Popup";

import "./EditorPopup.scss";
import { getFileContent, saveFileContent } from "../../api/api";

const DEFAULT_WIDTH = 800;
const DEFAULT_HEIGHT = 400;

export interface EditorData {
  title: string;

  // The url (path) of the editor to display.
  url: string;

  // The content of the editor to display.
  content: string;
}

interface EditorPopupProps {
  data: EditorData;
  onClose: () => void;
}

interface ConfirmDialogProps {
  message: string;
  onConfirm: () => void;
  onCancel: () => void;
}

const ConfirmDialog: React.FC<ConfirmDialogProps> = ({
  message,
  onConfirm,
  onCancel,
}) => {
  return (
    <Popup
      title="Confirmation"
      onClose={onCancel}
      preventFocusSteal={true}
      className="popup-confirm"
      resizable={false}
      initialWidth={400}
      initialHeight={200}
    >
      <div className="confirm-dialog-content">
        <p>{message}</p>
        <div className="confirm-dialog-actions">
          <button onClick={onCancel}>Cancel</button>
          <button onClick={onConfirm}>Ok</button>
        </div>
      </div>
    </Popup>
  );
};

const EditorPopup: React.FC<EditorPopupProps> = ({ data, onClose }) => {
  const [fileContent, setFileContent] = useState(data.content);

  const [showConfirmDialog, setShowConfirmDialog] = useState(false);
  const [pendingAction, setPendingAction] = useState<null | (() => void)>(null);

  // Fetch the specified file content from the backend.
  useEffect(() => {
    const fetchFileContent = async () => {
      try {
        const respData = await getFileContent(data.url);
        setFileContent(respData.content);
      } catch (error) {
        console.error("Failed to fetch file content:", error);
      }
    };

    fetchFileContent();
  }, [data.url]);

  const saveFile = async (content: string) => {
    try {
      await saveFileContent(data.url, content);
      console.log("File saved successfully");
      // We can add UI prompts, such as displaying a success notification.
    } catch (error) {
      console.error("Failed to save file content:", error);
      // We can add UI prompts, such as popping up an error notification.
    }
  };

  const handleActionWithOptionalConfirm = (
    action: () => void,
    needsConfirm: boolean
  ) => {
    if (needsConfirm) {
      setPendingAction(() => action);
      setShowConfirmDialog(true);
    } else {
      action();
    }
  };

  return (
    <>
      <Popup
        title={data.title}
        onClose={onClose}
        preventFocusSteal={true}
        className="popup-editor"
        resizable={true}
        initialWidth={DEFAULT_WIDTH}
        initialHeight={DEFAULT_HEIGHT}
      >
        <Editor
          height="100%"
          defaultLanguage="json"
          value={fileContent}
          options={{
            readOnly: false,
            automaticLayout: true,
          }}
          onChange={(value) => setFileContent(value || "")}
          onMount={(editor) => {
            editor.focus(); // Set the keyboard focus to the editor.

            editor.addAction({
              id: "save-file",
              label: "Save",
              contextMenuGroupId: "navigation",
              contextMenuOrder: 1.5,
              run: async (ed) => {
                // When the user clicks this menu item, first display a
                // confirmation popup.
                handleActionWithOptionalConfirm(
                  () => {
                    // Confirm before saving.
                    const currentContent = ed.getValue();
                    setFileContent(currentContent);
                    saveFile(currentContent);
                  },
                  true // Display a confirmation popup.
                );
              },
            });
          }}
        />
      </Popup>

      {/* Conditional Rendering Confirmation Popup. */}
      {showConfirmDialog && pendingAction && (
        <ConfirmDialog
          message="Are you sure you want to save this file?"
          onConfirm={() => {
            setShowConfirmDialog(false);
            pendingAction();
            setPendingAction(null);
          }}
          onCancel={() => {
            setShowConfirmDialog(false);
            setPendingAction(null);
          }}
        />
      )}
    </>
  );
};

export default EditorPopup;
