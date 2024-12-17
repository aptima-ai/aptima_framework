//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
import React, { useEffect, useState, useRef } from "react";

import AppBar from "./components/AppBar/AppBar";
import FlowCanvas, { FlowCanvasRef } from "./flow/FlowCanvas";
import SettingsPopup from "./components/SettingsPopup/SettingsPopup";
import { useTheme } from "./hooks/useTheme";

import "./theme/index.scss";
import { fetchDesignerVersion } from "./api/api";

const App: React.FC = () => {
  const [version, setVersion] = useState<string>("");
  const [error, setError] = useState<string>("");
  const [showSettings, setShowSettings] = useState(false);
  const { theme, setTheme } = useTheme();

  const flowCanvasRef = useRef<FlowCanvasRef>(null);

  // Get the version of tman.
  useEffect(() => {
    fetchDesignerVersion()
      .then((version) => {
        setVersion(version);
      })
      .catch((err) => {
        console.error("Failed to fetch version:", err);
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
