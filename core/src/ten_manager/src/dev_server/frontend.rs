//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
use rust_embed::RustEmbed;

#[derive(RustEmbed)]
#[folder = "frontend/dist/"] // Points to the frontend build output directory.
pub struct Asset;
