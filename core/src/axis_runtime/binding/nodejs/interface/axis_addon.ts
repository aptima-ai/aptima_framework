//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
let addon = {};

try {
  addon = require("libaxis_runtime_nodejs");
} catch (e) {
  console.error(`Failed to load libaxis_runtime_nodejs module: ${e}`);
}

export default addon as any;
