//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
import * as React from "react";
import { FolderClosedIcon, FileIcon, FolderOpenIcon } from "lucide-react";

import { Button } from "@/components/ui/Button";
import { SpinnerLoading } from "@/components/Status/Loading";
import { cn } from "@/lib/utils";
import type { IFMItem } from "@/components/FileManager/utils";
import {
  EFMItemType,
  calculateDirDepth,
  getSelectedPathAndNeighbors,
} from "@/components/FileManager/utils";

const DEFAULT_SELECTED_PATH = "/";

export function ThreeColumnFileManager(props: {
  allowSelectTypes?: EFMItemType[];
  onSelect?: (path: string) => void;
  data?: IFMItem[][];
  selectedPath?: string;
  className?: string;
  isLoading?: boolean;
}) {
  const {
    onSelect,
    data,
    selectedPath = DEFAULT_SELECTED_PATH,
    allowSelectTypes,
    className,
    isLoading,
  } = props;

  const [colsMemo, metaMemo] = React.useMemo(() => {
    const cols = getSelectedPathAndNeighbors(data || [], selectedPath);
    const selectedPathDepth = calculateDirDepth(selectedPath);
    if (selectedPathDepth === 1) {
      return [
        [cols.current, cols.next1, cols.next2],
        { selectedPathDepth, currentSelectedCol: 0 },
      ];
    } else if (selectedPathDepth === 2) {
      return [
        [cols.prev1, cols.current, cols.next1],
        { selectedPathDepth, currentSelectedCol: 1 },
      ];
    } else {
      // if selectedPathDepth > 2, we need to check if the last path is empty
      const colChild = cols.next1;
      if (colChild.length === 0) {
        return [
          [cols.prev2, cols.prev1, cols.current],
          { selectedPathDepth, currentSelectedCol: 2 },
        ];
      }
    }
    return [
      [cols.prev1, cols.current, cols.next1],
      { selectedPathDepth, currentSelectedCol: 1 },
    ];
  }, [data, selectedPath]);

  const handleSelect = (path: string) => {
    onSelect?.(path);
  };

  return (
    <div className={cn("w-full h-full space-y-2", className)}>
      <div
        className={cn(
          "w-full h-10 bg-gray-50 dark:bg-gray-900 rounded-lg",
          "px-4 flex items-center"
        )}
      >
        <span className="text-xs text-gray-500 dark:text-gray-400">
          {selectedPath}
        </span>
      </div>
      <div
        className={cn(
          "flex py-2 w-full h-[calc(100%-3rem)]",
          "bg-gray-50 dark:bg-gray-900 rounded-lg"
        )}
      >
        <FileManagerColumn
          className="w-1/3 border-r border-gray-300"
          isLoading={isLoading && metaMemo.currentSelectedCol === 0}
        >
          {colsMemo[0].map((item) => (
            <FMColumnItem
              key={item.path}
              data={item}
              onClick={() => handleSelect(item.path)}
              selectStatus={
                selectedPath.startsWith(item.path)
                  ? metaMemo.currentSelectedCol === 0
                    ? "selected"
                    : "selected-parent"
                  : "unselected"
              }
              disabled={
                allowSelectTypes ? !allowSelectTypes.includes(item.type) : false
              }
            />
          ))}
        </FileManagerColumn>
        <FileManagerColumn
          className="w-1/3 border-r border-gray-300"
          isLoading={isLoading && metaMemo.currentSelectedCol === 1}
        >
          {colsMemo[1].map((item) => (
            <FMColumnItem
              key={item.path}
              data={item}
              onClick={() => handleSelect(item.path)}
              selectStatus={
                selectedPath.startsWith(item.path)
                  ? metaMemo.currentSelectedCol === 1
                    ? "selected"
                    : "selected-parent"
                  : "unselected"
              }
              disabled={
                allowSelectTypes ? !allowSelectTypes.includes(item.type) : false
              }
            />
          ))}
        </FileManagerColumn>
        <FileManagerColumn
          className="w-1/3 px-2 overflow-y-auto"
          isLoading={isLoading && metaMemo.currentSelectedCol === 2}
        >
          {colsMemo[2].map((item) => (
            <FMColumnItem
              key={item.path}
              data={item}
              onClick={() => handleSelect(item.path)}
              selectStatus={
                selectedPath.startsWith(item.path) ? "selected" : "unselected"
              }
              disabled={
                allowSelectTypes ? !allowSelectTypes.includes(item.type) : false
              }
            />
          ))}
        </FileManagerColumn>
      </div>
    </div>
  );
}

function FileManagerColumn(props: {
  className?: string;
  children?: React.ReactNode;
  isLoading?: boolean;
}) {
  const { className, children, isLoading } = props;

  return (
    <ul
      className={cn(
        "px-2 overflow-y-auto",
        {
          "flex items-center justify-center text-gray-500 dark:text-gray-400":
            isLoading,
        },
        className
      )}
    >
      {isLoading ? <SpinnerLoading /> : children}
    </ul>
  );
}

function FMColumnItem(props: {
  data: IFMItem;
  onClick?: () => void;
  className?: string;
  selectStatus?: "unselected" | "selected" | "selected-parent";
  disabled?: boolean;
}) {
  const {
    data,
    onClick,
    className,
    selectStatus = "unselected",
    disabled,
  } = props;

  return (
    <li key={data.path}>
      <Button
        variant="ghost"
        size="sm"
        className={cn(
          "w-full justify-start",
          "hover:bg-gray-200 dark:hover:bg-gray-900",
          {
            "bg-gray-300 dark:bg-gray-800": selectStatus === "selected",
            "bg-gray-200 dark:bg-gray-900": selectStatus === "selected-parent",
          },
          className
        )}
        onClick={onClick}
        disabled={disabled}
      >
        {data.type === EFMItemType.FOLDER &&
          (selectStatus === "selected" ||
            selectStatus === "selected-parent") && (
            <FolderOpenIcon className="" />
          )}
        {data.type === EFMItemType.FOLDER && selectStatus === "unselected" && (
          <FolderClosedIcon className="" />
        )}
        {data.type !== EFMItemType.FOLDER && <FileIcon className="" />}
        <span className="overflow-hidden text-ellipsis whitespace-nowrap">
          {data.name} {selectStatus === "selected" ? "(Selected)" : ""}
        </span>
      </Button>
    </li>
  );
}
