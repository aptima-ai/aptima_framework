//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
pub mod predefined_graphs;

use std::{collections::HashSet, path::Path};

use anyhow::Result;

use crate::{config::TmanConfig, registry::found_result::RegistryPackageData};
use ten_rust::pkg_info::{
    dependencies::get_pkg_dependencies_from_manifest_dependencies,
    get_all_existed_pkgs_info_of_app_to_hashset, pkg_identity::PkgIdentity,
    supports::get_pkg_supports_from_manifest_supports, PkgInfo,
};

pub fn pkg_info_from_find_package_data(
    package_data: &RegistryPackageData,
) -> Result<PkgInfo> {
    Ok(PkgInfo {
        pkg_identity: PkgIdentity {
            pkg_type: package_data.pkg_type.clone(),
            name: package_data.name.clone(),
        },
        version: package_data.version.clone(),
        dependencies: get_pkg_dependencies_from_manifest_dependencies(
            &package_data.dependencies,
        )?,
        api: None,
        predefined_graphs: vec![],
        supports: get_pkg_supports_from_manifest_supports(
            &package_data.supports,
        )?,
        compatible_score: -1,

        is_local_installed: false,
        url: "".to_string(),
        hash: package_data.gen_hash_hex()?,

        manifest: None,
        property: None,
        schema_store: None,
    })
}

fn tman_get_all_existed_pkgs_info_of_app_to_hashset(
    tman_config: &TmanConfig,
    app_path: &Path,
) -> Result<HashSet<PkgInfo>> {
    let pkgs_info = get_all_existed_pkgs_info_of_app_to_hashset(app_path);
    crate::log::tman_verbose_println!(tman_config, "{:?}", pkgs_info);
    pkgs_info
}

pub fn tman_get_all_existed_pkgs_info_of_app(
    tman_config: &TmanConfig,
    app_path: &Path,
) -> Result<Vec<PkgInfo>> {
    let pkgs_info = tman_get_all_existed_pkgs_info_of_app_to_hashset(
        tman_config,
        app_path,
    )?;
    Ok(pkgs_info.into_iter().collect())
}
