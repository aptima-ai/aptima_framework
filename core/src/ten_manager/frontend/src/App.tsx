//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
import React, { useEffect, useState } from "react";
import { useTranslation } from "react-i18next";

interface ApiResponse<T> {
  status: string;
  data: T;
  meta?: any;
}

interface DevServerVersion {
  version: string;
}

const App: React.FC = () => {
  const { t, i18n } = useTranslation("common");
  const [version, setVersion] = useState<string>("");
  const [error, setError] = useState<string>("");

  useEffect(() => {
    fetch("/api/dev-server/v1/version")
      .then((response) => {
        if (!response.ok) {
          throw new Error(`HTTP error! status: ${response.status}`);
        }
        return response.json();
      })
      .then((payload: ApiResponse<DevServerVersion>) => {
        if (payload.status === "ok" && payload.data.version) {
          setVersion(payload.data.version);
        } else {
          throw new Error("Failed to fetch version information.");
        }
      })
      .catch((err) => {
        console.error("Error fetching API:", err);
        setError("Failed to fetch version.");
      });
  }, []);

  const changeLanguage = (lng: string) => {
    i18n.changeLanguage(lng);
  };

  return (
    <div className="App">
      <div style={{ marginBottom: "20px" }}>
        <button onClick={() => changeLanguage("en")}>English</button>
        <button onClick={() => changeLanguage("zh_cn")}>中文</button>
      </div>

      {error ? (
        <p style={{ color: "red" }}>{t("error_fetching")}</p>
      ) : (
        <h1>{t("current_version", { version })}</h1>
      )}
    </div>
  );
};

export default App;
