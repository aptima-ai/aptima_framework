//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
pub mod api;
pub mod dependency;
pub mod publish;
pub mod support;

use std::{fmt, fs, path::Path, str::FromStr};

use anyhow::{Context, Result};
use serde::{Deserialize, Serialize};

use crate::pkg_info::utils::read_file_to_string;
use crate::{json_schema, pkg_info::constants::MANIFEST_JSON_FILENAME};
use api::ManifestApi;
use dependency::ManifestDependency;
use publish::PackageConfig;
use support::ManifestSupport;

use super::pkg_type_and_name::PkgTypeAndName;

// Define a structure that mirrors the structure of the JSON file.
#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct Manifest {
    #[serde(flatten)]
    pub type_and_name: PkgTypeAndName,

    pub version: String,

    #[serde(skip_serializing_if = "Option::is_none")]
    pub dependencies: Option<Vec<ManifestDependency>>,

    // Note: For future extensions, use the 'features' field to describe the
    // functionality of each package.
    #[serde(skip_serializing_if = "Option::is_none")]
    pub supports: Option<Vec<ManifestSupport>>,

    #[serde(skip_serializing_if = "Option::is_none")]
    pub api: Option<ManifestApi>,

    #[serde(skip_serializing_if = "Option::is_none")]
    pub package: Option<PackageConfig>,
}

impl FromStr for Manifest {
    type Err = anyhow::Error;

    fn from_str(s: &str) -> Result<Self, Self::Err> {
        let mut manifest: Manifest = serde_json::from_str(s)?;

        manifest.validate_and_complete()?;

        // Return the parsed data.
        Ok(manifest)
    }
}

impl fmt::Display for Manifest {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        match serde_json::to_string_pretty(&self) {
            Ok(json_str) => write!(f, "{}", json_str),
            Err(_) => write!(f, "Failed to serialize manifest"),
        }
    }
}

impl Manifest {
    pub fn validate_and_complete(&mut self) -> Result<()> {
        Ok(())
    }
}

pub fn dump_manifest_str_to_file<P: AsRef<Path>>(
    manifest_str: &String,
    manifest_file_path: P,
) -> Result<()> {
    fs::write(manifest_file_path, manifest_str)?;
    Ok(())
}

pub fn parse_manifest_from_file<P: AsRef<Path>>(
    manifest_file_path: P,
) -> Result<Manifest> {
    // Read the contents of the manifest.json file.
    let content = read_file_to_string(manifest_file_path)?;

    Manifest::from_str(&content)
}

pub fn parse_manifest_in_folder(folder_path: &Path) -> Result<Manifest> {
    // Path to the manifest.json file.
    let manifest_path = folder_path.join(MANIFEST_JSON_FILENAME);

    // Read and parse the manifest.json file.
    let manifest = crate::pkg_info::manifest::parse_manifest_from_file(
        manifest_path.clone(),
    )
    .with_context(|| format!("Failed to load {}.", manifest_path.display()))?;

    // Validate the manifest schema only if it is present.
    let manifest_path_str = manifest_path.to_owned();
    json_schema::ten_validate_manifest_json_file(
        manifest_path_str.to_str().unwrap(),
    )
    .with_context(|| {
        format!("Failed to validate {}.", manifest_path.display())
    })?;

    Ok(manifest)
}

#[cfg(test)]
mod tests {
    use crate::pkg_info::pkg_type::PkgType;

    use super::*;

    #[test]
    fn test_extension_manifest_from_str() {
        let manifest_str = include_str!(
            "test_data_embed/test_extension_manifest_from_str.json"
        );

        let result: Result<Manifest> = manifest_str.parse();
        assert!(result.is_ok());

        let manifest = result.unwrap();
        assert_eq!(manifest.type_and_name.pkg_type, PkgType::Extension);

        let cmd_in = manifest.api.unwrap().cmd_in.unwrap();
        assert_eq!(cmd_in.len(), 1);

        let required = cmd_in[0].required.as_ref();
        assert!(required.is_some());
        assert_eq!(required.unwrap().len(), 1);
    }
}
