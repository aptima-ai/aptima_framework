//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
import React, { useEffect, useState, useRef } from "react";
import { useTranslation } from "react-i18next";
import AppBar from "./components/AppBar/AppBar";
import FlowCanvas, { FlowCanvasRef } from "./flow/FlowCanvas";
import SettingsPopup from "./components/SettingsPopup/SettingsPopup";
import { useTheme } from "./hooks/useTheme";
import "./theme/index.scss";

interface ApiResponse<T> {
  status: string;
  data: T;
  meta?: any;
}

interface DevServerVersion {
  version: string;
}

const App: React.FC = () => {
  const { t } = useTranslation("common");
  const [version, setVersion] = useState<string>("");
  const [error, setError] = useState<string>("");
  const [showSettings, setShowSettings] = useState(false);
  const { theme, setTheme } = useTheme();

  const flowCanvasRef = useRef<FlowCanvasRef>(null);

  // Get the version of tman.
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

  const handleAutoLayout = () => {
    flowCanvasRef.current?.performAutoLayout();
  };

  return (
    <div className={`theme-${theme}`}>
      <AppBar
        version={version}
        error={error}
        onOpenSettings={() => setShowSettings(true)}
        onAutoLayout={handleAutoLayout}
      />
      <FlowCanvas ref={flowCanvasRef} />
      {showSettings && (
        <SettingsPopup
          theme={theme}
          onChangeTheme={setTheme}
          onClose={() => setShowSettings(false)}
        />
      )}
    </div>
  );
};

export default App;
