//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
import * as React from "react";
import { toast } from "sonner";
import { FolderOpenIcon } from "lucide-react";

import Popup from "@/components/Popup/Popup";
import {
  NavigationMenuContent,
  NavigationMenuItem,
  NavigationMenuLink,
  NavigationMenuTrigger,
} from "@/components/ui/NavigationMenu";
import { Button } from "@/components/ui/Button";
import { cn } from "@/lib/utils";
import { ThreeColumnFileManager } from "@/components/FileManager/AppFolder";
import {
  baseDirEntriesToIFMItems,
  fmItemsToFMArray,
  type IFMItem,
  EFMItemType,
} from "@/components/FileManager/utils";
import { useDirList } from "@/api/services/fileSystem";

interface FileMenuProps {
  defaultBaseDir?: string;
  onSetBaseDir: (folderPath: string) => void;
}

export function FileMenu(props: FileMenuProps) {
  const { defaultBaseDir = "/", onSetBaseDir } = props;

  const [isFolderPathModalOpen, setIsFolderPathModalOpen] =
    React.useState<boolean>(false);
  const [folderPath, setFolderPath] = React.useState<string>(defaultBaseDir);
  const [fmItems, setFmItems] = React.useState<IFMItem[][]>([]);

  const { data, error, isLoading } = useDirList(folderPath);

  const handleManualOk = async () => {
    if (!folderPath.trim()) {
      toast.error("The folder path cannot be empty.");
      return;
    }

    console.log("[file-menu] folderPath set to", folderPath);
    onSetBaseDir(folderPath.trim());
    setIsFolderPathModalOpen(false);
  };

  React.useEffect(() => {
    if (!data?.entries) {
      return;
    }
    const currentFmItems = baseDirEntriesToIFMItems(data.entries);
    const fmArray = fmItemsToFMArray(currentFmItems, fmItems);
    setFmItems(fmArray);
    // Suppress the warning about the dependency array.
    // <fmItems> should not be a dependency.
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [data, folderPath]);

  React.useEffect(() => {
    if (error) {
      toast.error("Failed to load the folder.", {
        description: error?.message,
      });
    }
  }, [error]);

  return (
    <>
      <NavigationMenuItem>
        <NavigationMenuTrigger className="submenu-trigger">
          File
        </NavigationMenuTrigger>
        <NavigationMenuContent
          className={cn("flex flex-col items-center px-1 py-1.5 gap-1.5")}
        >
          <NavigationMenuLink asChild>
            <Button
              className="w-full justify-start"
              variant="ghost"
              onClick={() => setIsFolderPathModalOpen(true)}
            >
              <FolderOpenIcon className="w-4 h-4 me-2" />
              Open app folder
            </Button>
          </NavigationMenuLink>
        </NavigationMenuContent>
      </NavigationMenuItem>
      {isFolderPathModalOpen && (
        <Popup
          title="Open App Folder"
          onClose={() => setIsFolderPathModalOpen(false)}
          resizable={false}
          initialWidth={600}
          initialHeight={400}
          onCollapseToggle={() => {}}
        >
          <div className="flex flex-col gap-2 w-full h-full">
            <ThreeColumnFileManager
              data={fmItems}
              allowSelectTypes={[EFMItemType.FOLDER]}
              className="w-full h-[calc(100%-3rem)]"
              onSelect={(path) => setFolderPath(path)}
              selectedPath={folderPath}
              isLoading={isLoading}
            />
            <div className="flex justify-end h-fit gap-2">
              <Button
                variant="outline"
                onClick={() => setIsFolderPathModalOpen(false)}
              >
                Cancel
              </Button>
              <Button onClick={handleManualOk}>OK</Button>
            </div>
          </div>
        </Popup>
      )}
    </>
  );
}
