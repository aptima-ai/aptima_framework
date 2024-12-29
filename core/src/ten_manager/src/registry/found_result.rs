//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
use std::path::PathBuf;

use anyhow::Result;
use serde::{Deserialize, Serialize};

use ten_rust::pkg_info::dependencies::PkgDependency;
use ten_rust::pkg_info::manifest::Manifest;
use ten_rust::pkg_info::pkg_basic_info::PkgBasicInfo;
use ten_rust::pkg_info::PkgInfo;

#[derive(Debug, Serialize, Deserialize)]
pub struct PkgRegistryInfo {
    #[serde(flatten)]
    pub basic_info: PkgBasicInfo,

    #[serde(with = "dependencies_conversion")]
    pub dependencies: Vec<PkgDependency>,

    pub hash: String,
}

mod dependencies_conversion {
    use serde::{Deserialize, Deserializer, Serialize, Serializer};
    use ten_rust::pkg_info::{
        dependencies::PkgDependency, manifest::dependency::ManifestDependency,
    };

    pub fn serialize<S>(
        deps: &[PkgDependency],
        serializer: S,
    ) -> Result<S::Ok, S::Error>
    where
        S: Serializer,
    {
        let manifest_deps: Vec<ManifestDependency> =
            deps.iter().cloned().map(|dep| (&dep).into()).collect();
        manifest_deps.serialize(serializer)
    }

    pub fn deserialize<'de, D>(
        deserializer: D,
    ) -> Result<Vec<PkgDependency>, D::Error>
    where
        D: Deserializer<'de>,
    {
        let manifest_deps: Vec<ManifestDependency> =
            Vec::deserialize(deserializer)?;
        manifest_deps
            .into_iter()
            .map(|dep| (&dep).try_into().map_err(serde::de::Error::custom))
            .collect()
    }
}

impl TryFrom<&Manifest> for PkgRegistryInfo {
    type Error = anyhow::Error;

    fn try_from(manifest: &Manifest) -> Result<Self> {
        let pkg_info = PkgInfo::from_metadata(manifest, &None)?;
        Ok((&pkg_info).into())
    }
}

impl From<&PkgInfo> for PkgRegistryInfo {
    fn from(pkg_info: &PkgInfo) -> Self {
        PkgRegistryInfo {
            basic_info: PkgBasicInfo::from(pkg_info),
            dependencies: pkg_info.dependencies.clone(),
            hash: pkg_info.hash.clone(),
        }
    }
}

impl From<&PkgRegistryInfo> for PkgInfo {
    fn from(pkg_registry_info: &PkgRegistryInfo) -> Self {
        let mut pkg_info = PkgInfo {
            basic_info: pkg_registry_info.basic_info.clone(),
            dependencies: pkg_registry_info.dependencies.clone(),
            api: None,
            compatible_score: -1,

            is_local_installed: false,
            url: String::new(),
            hash: String::new(),

            manifest: None,
            property: None,
            schema_store: None,
        };

        pkg_info.hash = pkg_info.gen_hash_hex();

        pkg_info
    }
}

#[derive(Debug, Serialize, Deserialize)]
pub struct FoundResult {
    pub url: PathBuf,
    pub pkg_registry_info: PkgRegistryInfo,
}
