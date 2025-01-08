//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
import React, { useEffect, useState, useContext } from "react";
import Editor from "@monaco-editor/react";
import { toast } from "sonner";
import { useTranslation } from "react-i18next";

import Popup from "@/components/Popup/Popup";
import { getFileContent, putFileContent } from "@/api/services/fileSystem";
import { ThemeProviderContext } from "@/components/theme-context";
import { Button } from "@/components/ui/Button";

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
  const { t } = useTranslation();

  return (
    <Popup
      title={t("action.confirm")}
      onClose={onCancel}
      preventFocusSteal={true}
      resizable={false}
      initialWidth={288}
      initialHeight={180}
    >
      <div className="flex flex-col items-center justify-center h-full mx-auto">
        <p className="text-sm text-foreground">{message}</p>
        <div className="flex items-center gap-4 mt-6 w-full justify-end">
          <Button variant="outline" size="sm" onClick={onCancel}>
            {t("action.cancel")}
          </Button>
          <Button variant="default" size="sm" onClick={onConfirm}>
            {t("action.ok")}
          </Button>
        </div>
      </div>
    </Popup>
  );
};

const EditorPopup: React.FC<EditorPopupProps> = ({ data, onClose }) => {
  const [fileContent, setFileContent] = useState(data.content);
  const [showConfirmDialog, setShowConfirmDialog] = useState(false);
  const [pendingAction, setPendingAction] = useState<null | (() => void)>(null);

  const { t } = useTranslation();

  // Fetch the specified file content from the backend.
  useEffect(() => {
    const fetchFileContent = async () => {
      try {
        const respData = await getFileContent(data.url);
        setFileContent(respData.content);
      } catch (error) {
        console.error("Failed to fetch file content:", error);
        toast.error("Failed to fetch file content");
      }
    };

    fetchFileContent();
  }, [data.url]);

  const saveFile = async (content: string) => {
    try {
      await putFileContent(data.url, { content });
      console.log(t("toast.saveFileSuccess"));
      toast.success(t("toast.saveFileSuccess"));
      // We can add UI prompts, such as displaying a success notification.
    } catch (error: unknown) {
      console.error("Failed to save file content:", error);
      toast.error(t("toast.saveFileFailed"), {
        description:
          error instanceof Error ? error.message : t("toast.saveFileFailed"),
      });
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

  const { theme } = useContext(ThemeProviderContext);

  return (
    <>
      <Popup
        title={data.title}
        onClose={onClose}
        preventFocusSteal={true}
        resizable={true}
        initialWidth={DEFAULT_WIDTH}
        initialHeight={DEFAULT_HEIGHT}
        contentClassName="p-0"
      >
        <div className="p-0 box-border flex flex-col w-full h-full">
          <Editor
            theme={theme === "dark" ? "vs-dark" : "vs-light"}
            height="100%"
            defaultLanguage="json"
            value={fileContent}
            options={{
              readOnly: false,
              automaticLayout: true,
            }}
            onChange={(value) => setFileContent(value || "")}
            onMount={(editor) => {
              // --- set context menu actions ---
              // reference: https://github.com/microsoft/monaco-editor/issues/1280#issuecomment-2420136963
              const keepIds = [
                "editor.action.clipboardCopyAction",
                "editor.action.clipboardCutAction",
                "editor.action.clipboardPasteAction",
                "editor.action.formatDocument",
                "vs.editor.ICodeEditor:1:save-file",
                "vs.actions.separator",
              ];
              const contextmenu = editor.getContribution(
                "editor.contrib.contextmenu"
              );
              // eslint-disable-next-line @typescript-eslint/no-explicit-any
              const realMethod = (contextmenu as any)._getMenuActions;
              // eslint-disable-next-line @typescript-eslint/no-explicit-any
              (contextmenu as any)._getMenuActions = function (...args: any[]) {
                const items = realMethod.apply(contextmenu, args);
                const filteredItems = items.filter(function (item: {
                  id: string;
                }) {
                  return keepIds.includes(item.id);
                });
                // Remove separator if it's the last item
                if (
                  filteredItems.length > 0 &&
                  filteredItems[filteredItems.length - 1].id ===
                    "vs.actions.separator"
                ) {
                  filteredItems.pop();
                }
                return filteredItems;
              };

              // --- set keyboard focus to the editor ---
              editor.focus();

              // --- add save-file action ---
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
        </div>
      </Popup>

      {/* Conditional Rendering Confirmation Popup. */}
      {showConfirmDialog && pendingAction && (
        <ConfirmDialog
          message={t("popup.editor.confirmSaveFile")}
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
