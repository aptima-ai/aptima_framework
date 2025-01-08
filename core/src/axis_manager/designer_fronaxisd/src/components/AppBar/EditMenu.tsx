//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
import { useTranslation } from "react-i18next";
import { FolderOpenIcon, MoveIcon } from "lucide-react";

import {
  NavigationMenuContent,
  NavigationMenuItem,
  NavigationMenuLink,
  NavigationMenuTrigger,
} from "@/components/ui/NavigationMenu";
import { Button } from "@/components/ui/Button";
import { cn } from "@/lib/utils";

interface EditMenuProps {
  onAutoLayout: () => void;
  onOpenExistingGraph: () => void;
}

export function EditMenu(props: EditMenuProps) {
  const { onAutoLayout, onOpenExistingGraph } = props;
  const { t } = useTranslation();

  return (
    <NavigationMenuItem>
      <NavigationMenuTrigger className="submenu-trigger">
        {t("header.menu.edit")}
      </NavigationMenuTrigger>
      <NavigationMenuContent
        className={cn("flex flex-col items-center px-1 py-1.5 gap-1.5")}
      >
        <NavigationMenuLink asChild>
          <Button
            className="w-full justify-start"
            variant="ghost"
            onClick={onOpenExistingGraph}
          >
            <FolderOpenIcon />
            {t("header.menu.openExistingGraph")}
          </Button>
        </NavigationMenuLink>
        <NavigationMenuLink asChild>
          <Button
            className="w-full justify-start"
            variant="ghost"
            onClick={onAutoLayout}
          >
            <MoveIcon />
            {t("header.menu.autoLayout")}
          </Button>
        </NavigationMenuLink>
      </NavigationMenuContent>
    </NavigationMenuItem>
  );
}
