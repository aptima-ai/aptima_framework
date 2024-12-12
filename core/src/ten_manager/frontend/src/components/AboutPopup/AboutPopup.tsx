//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
import React from "react";
import Popup from "../Popup/Popup";
import "./AboutPopup.scss";

interface AboutPopupProps {
  onClose: () => void;
}

const AboutPopup: React.FC<AboutPopupProps> = ({ onClose }) => {
  return (
    <Popup title="About" onClose={onClose}>
      <div className="about-content">
        <p className="powered-by">Powered by TEN Framework.</p>
        <p>
          Official site:{" "}
          <a
            href="https://www.theten.ai/"
            target="_blank"
            rel="noopener noreferrer"
            className="about-link"
          >
            https://www.theten.ai/
          </a>
        </p>
        <p>
          Github:{" "}
          <a
            href="https://github.com/TEN-framework/"
            target="_blank"
            rel="noopener noreferrer"
            className="about-link"
          >
            https://github.com/TEN-framework/
          </a>
        </p>
      </div>
    </Popup>
  );
};

export default AboutPopup;
