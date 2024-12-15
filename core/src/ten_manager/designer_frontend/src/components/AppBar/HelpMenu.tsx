//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
import React, { useState } from "react";
import { FaInfoCircle } from "react-icons/fa";

import DropdownMenu, { DropdownMenuItem } from "./DropdownMenu";
import AboutPopup from "../AboutPopup/AboutPopup";

interface HelpMenuProps {
  isOpen: boolean;
  onClick: () => void;
  onHover: () => void;
  closeMenu: () => void;
}

const HelpMenu: React.FC<HelpMenuProps> = ({
  isOpen,
  onClick,
  onHover,
  closeMenu,
}) => {
  const [isAboutOpen, setIsAboutOpen] = useState(false);

  const openAbout = () => {
    setIsAboutOpen(true);
    closeMenu();
  };

  const closeAbout = () => {
    setIsAboutOpen(false);
  };

  const items: DropdownMenuItem[] = [
    {
      label: "About",
      icon: <FaInfoCircle />,
      onClick: openAbout,
    },
  ];

  return (
    <>
      <DropdownMenu
        title="Help"
        isOpen={isOpen}
        onClick={onClick}
        onHover={onHover}
        items={items}
      />

      {isAboutOpen && <AboutPopup onClose={closeAbout} />}
    </>
  );
};

export default HelpMenu;
