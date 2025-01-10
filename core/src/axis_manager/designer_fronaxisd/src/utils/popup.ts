//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
export const dispatchCustomNodeActionPopup = (
  action: string,
  source: string,
  target?: string
) => {
  if (typeof window !== "undefined") {
    window.dispatchEvent(
      new CustomEvent("customNodeAction", {
        detail: { action, source, target },
      })
    );
  }
};
