//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
import * as React from "react";
import { InfoIcon } from "lucide-react";
import { useTranslation } from "react-i18next";

import AboutPopup from "@/components/Popup/AboutPopup";
import {
  NavigationMenuContent,
  NavigationMenuItem,
  NavigationMenuLink,
  NavigationMenuTrigger,
} from "@/components/ui/NavigationMenu";
import { Button } from "@/components/ui/Button";
import { cn } from "@/lib/utils";

export function HelpMenu() {
  const { t } = useTranslation();
  const [isAboutOpen, setIsAboutOpen] = React.useState(false);

  const openAbout = () => {
    setIsAboutOpen(true);
  };

  const closeAbout = () => {
    setIsAboutOpen(false);
  };

  return (
    <>
      <NavigationMenuItem>
        <NavigationMenuTrigger className="submenu-trigger">
          {t("header.menu.help")}
        </NavigationMenuTrigger>
        <NavigationMenuContent
          className={cn("flex flex-col items-center px-1 py-1.5 gap-1.5")}
        >
          <NavigationMenuLink asChild>
            <Button
              className="w-full justify-start max-w-screen-sm"
              variant="ghost"
              onClick={openAbout}
            >
              <InfoIcon />
              {t("header.menu.about")}
            </Button>
          </NavigationMenuLink>
        </NavigationMenuContent>
      </NavigationMenuItem>

      {isAboutOpen && <AboutPopup onClose={closeAbout} />}
    </>
  );
}
