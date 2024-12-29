//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
pub mod predefined_graphs;

use std::path::Path;

use anyhow::Result;

use crate::config::TmanConfig;
use ten_rust::pkg_info::{
    get_all_existed_pkgs_info_of_app_to_hashmap, PkgInfo,
};

pub fn tman_get_all_existed_pkgs_info_of_app(
    tman_config: &TmanConfig,
    app_path: &Path,
) -> Result<Vec<PkgInfo>> {
    let pkgs_info = get_all_existed_pkgs_info_of_app_to_hashmap(app_path)?;
    crate::log::tman_verbose_println!(tman_config, "{:?}", pkgs_info);
    Ok(pkgs_info.into_values().collect())
}
