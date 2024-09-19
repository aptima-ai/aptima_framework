//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
pub mod api;
pub mod dependency;
pub mod publish;
pub mod support;

use std::{fmt, fs, path::Path, str::FromStr};

use anyhow::Result;
use serde::{Deserialize, Serialize};

use crate::pkg_info::utils::read_file_to_string;
use crate::{json_schema, pkg_info::constants::MANIFEST_JSON_FILENAME};
use api::ManifestApi;
use dependency::ManifestDependency;
use publish::PackageConfig;
use support::ManifestSupport;

use super::dependencies::get_pkg_dependencies_from_manifest_dependencies;
use super::hash::gen_hash_hex;
use super::supports::get_pkg_supports_from_manifest_supports;

// Define a structure that mirrors the structure of the JSON file.
#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct Manifest {
    #[serde(rename = "type")]
    pub pkg_type: String,
    pub name: String,
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

    pub fn gen_hash_hex(&self) -> Result<String> {
        let dependencies =
            self.dependencies.as_ref().map_or(Ok(vec![]), |d| {
                get_pkg_dependencies_from_manifest_dependencies(d)
            })?;

        let supports = get_pkg_supports_from_manifest_supports(&self.supports)?;

        gen_hash_hex(
            &self.pkg_type.parse()?,
            &self.name,
            &self.version.parse()?,
            &dependencies,
            &supports,
        )
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
    )?;

    // Validate the manifest schema only if it is present.
    let manifest_path_str = manifest_path.to_owned();
    let validation = json_schema::ten_validate_manifest_json_file(
        manifest_path_str.to_str().unwrap(),
    );

    if let Err(validation_err) = validation {
        return Err(anyhow::anyhow!(
            "Failed to validate {}, caused by {}",
            manifest_path.display(),
            validation_err
        ));
    }

    Ok(manifest)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_extension_manifest_from_str() {
        let manifest_str = include_str!(
            "test_data_embed/test_extension_manifest_from_str.json"
        );

        let result: Result<Manifest> = manifest_str.parse();
        assert!(result.is_ok());

        let manifest = result.unwrap();
        assert_eq!(manifest.pkg_type, "extension");

        let cmd_in = manifest.api.unwrap().cmd_in.unwrap();
        assert_eq!(cmd_in.len(), 1);

        let required = cmd_in[0].required.as_ref();
        assert!(required.is_some());
        assert_eq!(required.unwrap().len(), 1);
    }
}
