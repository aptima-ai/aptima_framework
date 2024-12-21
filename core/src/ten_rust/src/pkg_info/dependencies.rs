//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
use std::str::FromStr;

use anyhow::Result;
use semver::VersionReq;
use serde::{Deserialize, Serialize};

use super::{pkg_type::PkgType, pkg_type_and_name::PkgTypeAndName};
use crate::pkg_info::manifest::{dependency::ManifestDependency, Manifest};

#[derive(Clone, Debug, Serialize, Deserialize)]
pub struct PkgDependency {
    pub type_and_name: PkgTypeAndName,

    // The version requirement of this dependency, ex: the `version`
    // field declared in the `dependencies` section in the manifest.json, or
    // the `pkg@version` parameter in the command line.
    //
    // The `version_req_str` is the original value in string form, while
    // `version_req` is the result after being converted to `VersionReq`.
    pub version_req: VersionReq,
    pub version_req_str: String,
}

impl PkgDependency {
    pub fn new(
        pkg_type: PkgType,
        name: String,
        version_req: VersionReq,
    ) -> Self {
        PkgDependency {
            type_and_name: PkgTypeAndName { pkg_type, name },
            version_req_str: version_req.to_string(),
            version_req,
        }
    }
}

impl From<&PkgDependency> for PkgTypeAndName {
    fn from(dependency: &PkgDependency) -> Self {
        dependency.type_and_name.clone()
    }
}

impl TryFrom<&ManifestDependency> for PkgDependency {
    type Error = anyhow::Error;

    fn try_from(manifest_dep: &ManifestDependency) -> Result<Self> {
        Ok(PkgDependency {
            type_and_name: PkgTypeAndName {
                pkg_type: PkgType::from_str(&manifest_dep.pkg_type)?,
                name: manifest_dep.name.clone(),
            },
            version_req: VersionReq::parse(&manifest_dep.version)?,
            version_req_str: manifest_dep.version.clone(),
        })
    }
}

pub fn get_pkg_dependencies_from_manifest(
    manifest: &Manifest,
) -> Result<Vec<PkgDependency>> {
    match &manifest.dependencies {
        Some(manifest_dependencies) => {
            Ok(get_pkg_dependencies_from_manifest_dependencies(
                manifest_dependencies,
            )?)
        }
        None => Ok(vec![]),
    }
}

fn get_pkg_dependencies_from_manifest_dependencies(
    manifest_dependencies: &[ManifestDependency],
) -> Result<Vec<PkgDependency>> {
    manifest_dependencies
        .iter()
        .map(PkgDependency::try_from)
        .collect()
}

pub fn get_manifest_dependencies_from_pkg(
    pkg_dependencies: Vec<PkgDependency>,
) -> Vec<ManifestDependency> {
    pkg_dependencies.into_iter().map(|v| (&v).into()).collect()
}
