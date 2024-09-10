//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
use std::path::PathBuf;

use anyhow::Result;
use semver::Version;
use serde::{Deserialize, Serialize};

use ten_rust::pkg_info::manifest::{
    dependency::ManifestDependency, support::ManifestSupport,
};
use ten_rust::pkg_info::{
    dependencies::get_pkg_dependencies_from_manifest_dependencies,
    hash::gen_hash_hex, pkg_type::PkgType,
    supports::get_pkg_supports_from_manifest_supports,
};

#[derive(Debug, Serialize, Deserialize)]
pub struct RegistryPackageData {
    #[serde(rename = "type")]
    pub pkg_type: PkgType,

    pub name: String,
    pub version: Version,
    pub dependencies: Vec<ManifestDependency>,

    #[serde(skip_serializing_if = "Option::is_none")]
    pub supports: Option<Vec<ManifestSupport>>,

    pub hash: String,
}

impl RegistryPackageData {
    pub fn gen_hash_hex(&self) -> Result<String> {
        let dependencies = get_pkg_dependencies_from_manifest_dependencies(
            &self.dependencies,
        )?;

        let supports = get_pkg_supports_from_manifest_supports(&self.supports)?;

        gen_hash_hex(
            &self.pkg_type,
            &self.name,
            &self.version,
            &dependencies,
            &supports,
        )
    }
}

#[derive(Debug, Serialize, Deserialize)]
pub struct FoundResult {
    pub url: PathBuf,
    pub package_data: RegistryPackageData,
}
