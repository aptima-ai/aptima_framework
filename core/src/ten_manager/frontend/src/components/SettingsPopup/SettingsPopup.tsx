//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
import React from "react";
import Popup from "../Popup/Popup";
import "./SettingsPopup.scss";
import { useTranslation } from "react-i18next";

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
  const { t, i18n } = useTranslation();

  const toggleTheme = () => {
    onChangeTheme(theme === "light" ? "dark" : "light");
  };

  const handleLanguageChange = (
    event: React.ChangeEvent<HTMLSelectElement>
  ) => {
    const selectedLanguage = event.target.value;
    i18n.changeLanguage(selectedLanguage);
  };

  return (
    <Popup title={t("Settings")} onClose={onClose}>
      <div className="theme-toggle">
        <span>
          {t("Theme")}: {theme.charAt(0).toUpperCase() + theme.slice(1)}
        </span>{" "}
        <button onClick={toggleTheme}>
          {t("Switch to")} {theme === "light" ? t("Dark") : t("Light")}{" "}
          {t("Theme")}
        </button>
      </div>
      <div className="language-selector">
        <span>{t("Language")}: </span>
        <select value={i18n.language} onChange={handleLanguageChange}>
          <option value="en">English</option>
          <option value="zh_cn">简体中文</option>
        </select>
      </div>
    </Popup>
  );
};

export default SettingsPopup;
