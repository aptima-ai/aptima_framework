//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
pub mod predefined_graphs;

use std::path::Path;

use anyhow::Result;

use crate::{config::TmanConfig, registry::found_result::RegistryPackageData};
use ten_rust::pkg_info::{
    dependencies::get_pkg_dependencies_from_manifest_dependencies,
    get_all_existed_pkgs_info_of_app_to_hashmap, pkg_basic_info::PkgBasicInfo,
    PkgInfo,
};

pub fn pkg_info_from_find_package_data(
    package_data: &RegistryPackageData,
) -> Result<PkgInfo> {
    Ok(PkgInfo {
        basic_info: PkgBasicInfo::try_from(package_data)?,
        dependencies: get_pkg_dependencies_from_manifest_dependencies(
            &package_data.dependencies,
        )?,
        api: None,
        compatible_score: -1,

        is_local_installed: false,
        url: "".to_string(),
        hash: package_data.gen_hash_hex()?,

        manifest: None,
        property: None,
        schema_store: None,
    })
}

pub fn tman_get_all_existed_pkgs_info_of_app(
    tman_config: &TmanConfig,
    app_path: &Path,
) -> Result<Vec<PkgInfo>> {
    let pkgs_info = get_all_existed_pkgs_info_of_app_to_hashmap(app_path)?;
    crate::log::tman_verbose_println!(tman_config, "{:?}", pkgs_info);
    Ok(pkgs_info.into_values().collect())
}
