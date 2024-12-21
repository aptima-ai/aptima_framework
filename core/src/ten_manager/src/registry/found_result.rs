//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
use std::path::PathBuf;

use anyhow::Result;
use serde::{Deserialize, Serialize};

use ten_rust::pkg_info::dependencies::PkgDependency;
use ten_rust::pkg_info::hash::gen_hash_hex;
use ten_rust::pkg_info::manifest::Manifest;
use ten_rust::pkg_info::pkg_basic_info::PkgBasicInfo;
use ten_rust::pkg_info::PkgInfo;

#[derive(Debug, Serialize, Deserialize)]
pub struct RegistryPackageData {
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

impl RegistryPackageData {
    pub fn gen_hash_hex(&self) -> Result<String> {
        Ok(gen_hash_hex(
            &self.basic_info.type_and_name.pkg_type,
            &self.basic_info.type_and_name.name,
            &self.basic_info.version,
            &self.dependencies,
            &self.basic_info.supports,
        ))
    }
}

impl TryFrom<&Manifest> for RegistryPackageData {
    type Error = anyhow::Error;

    fn try_from(manifest: &Manifest) -> Result<Self> {
        let dependencies =
            manifest.dependencies.as_ref().map_or(Ok(vec![]), |v| {
                v.iter()
                    .map(|d| d.try_into())
                    .collect::<Result<Vec<PkgDependency>, anyhow::Error>>()
            })?;

        Ok(RegistryPackageData {
            basic_info: PkgBasicInfo::try_from(manifest)?,
            dependencies,
            hash: manifest.gen_hash_hex()?,
        })
    }
}

impl From<&PkgInfo> for RegistryPackageData {
    fn from(pkg_info: &PkgInfo) -> Self {
        RegistryPackageData {
            basic_info: PkgBasicInfo::from(pkg_info),
            dependencies: pkg_info.dependencies.clone(),
            hash: pkg_info.gen_hash_hex(),
        }
    }
}

#[derive(Debug, Serialize, Deserialize)]
pub struct FoundResult {
    pub url: PathBuf,
    pub package_data: RegistryPackageData,
}
