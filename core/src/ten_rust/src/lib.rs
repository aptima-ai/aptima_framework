//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
mod bindings;
mod interface;
pub mod json;
pub mod json_schema;
pub mod pkg_info;

// In the schema/ folder, the Rust API is mainly automatically generated from C
// headers. There's no need to generate C code in reverse from Rust.
// Additionally, if including schema/ folder here, it would result in duplicate
// declarations.
/// cbindgen:ignore
mod schema;
