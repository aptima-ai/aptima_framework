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

const EditorPopup: React.FC<EditorPopupProps> = ({ data, onClose }) => {
  const [fileContent, setFileContent] = useState(data.content);

  // Fetch the specified file content from the backend.
  useEffect(() => {
    const fetchFileContent = async () => {
      try {
        const encodedUrl = encodeURIComponent(data.url);
        const response = await fetch(
          `/api/dev-server/v1/file-content/${encodedUrl}`
        );
        if (!response.ok) throw new Error("Failed to fetch file content");
        const respData = await response.json();
        setFileContent(respData.data.content);
      } catch (error) {
        console.error("Failed to fetch file content:", error);
      }
    };

    fetchFileContent();
  }, [data.url]);

  return (
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
        onMount={(editor) => {
          editor.focus(); // Set the keyboard focus to the editor.
        }}
      />
    </Popup>
  );
};

export default EditorPopup;
