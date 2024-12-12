//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
import React from "react";
import Popup from "../Popup/Popup";
import "./SettingsPopup.scss";

interface SettingsPopupProps {
  theme: string;
  onChangeTheme: (theme: string) => void;
  onClose: () => void;
}

const SettingsPopup: React.FC<SettingsPopupProps> = ({
  theme,
  onChangeTheme,
  onClose,
}) => {
  const toggleTheme = () => {
    onChangeTheme(theme === "light" ? "dark" : "light");
  };

  return (
    <Popup title="Settings" onClose={onClose}>
      <div className="theme-toggle">
        <span>Theme: {theme.charAt(0).toUpperCase() + theme.slice(1)}</span>
        <button onClick={toggleTheme}>
          Switch to {theme === "light" ? "Dark" : "Light"} Theme
        </button>
      </div>
    </Popup>
  );
};

export default SettingsPopup;
