//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
import React, { useState } from "react";
import DropdownMenu from "./DropdownMenu";
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
  const [isDocumentationOpen, setIsDocumentationOpen] = useState(false);
  const [isAboutOpen, setIsAboutOpen] = useState(false);

  const openDocumentation = () => {
    setIsDocumentationOpen(true);
    closeMenu();
  };

  const closeDocumentation = () => {
    setIsDocumentationOpen(false);
  };

  const openAbout = () => {
    setIsAboutOpen(true);
    closeMenu();
  };

  const closeAbout = () => {
    setIsAboutOpen(false);
  };

  return (
    <>
      <DropdownMenu
        title="Help"
        isOpen={isOpen}
        onClick={onClick}
        onHover={onHover}
      >
        <div className="menu-item" onClick={openAbout}>
          About
        </div>
      </DropdownMenu>

      {isAboutOpen && <AboutPopup onClose={closeAbout} />}
    </>
  );
};

export default HelpMenu;
