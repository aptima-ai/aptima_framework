//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
mod bindings;
pub mod interface;
pub mod json;
pub mod json_schema;
pub mod pkg_info;

// In the schema/ folder, the Rust API is mainly automatically generated from C
// headers. There's no need to generate C code in reverse from Rust.
// Additionally, if including schema/ folder here, it would result in duplicate
// declarations.
/// cbindgen:ignore
mod schema;
