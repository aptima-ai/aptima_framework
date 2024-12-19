//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
use std::{
    hash::{Hash, Hasher},
    str::FromStr,
};

use anyhow::Result;
use semver::{Version, VersionReq};
use serde::{Deserialize, Serialize};

use super::{pkg_type::PkgType, pkg_type_and_name::PkgTypeAndName};
use crate::pkg_info::manifest::{dependency::ManifestDependency, Manifest};

#[derive(Clone, Debug, Serialize, Deserialize)]
pub struct PkgDependency {
    pub pkg_type: PkgType,
    pub name: String,

    // The version requirement of this dependency, ex: the `version`
    // field declared in the `dependencies` section in the manifest.json, or
    // the `pkg@version` parameter in the command line.
    //
    // The `version_req_str` is the original value in string form, while
    // `version_req` is the result after being converted to `VersionReq`.
    pub version_req: VersionReq,
    pub version_req_str: Option<String>,
}

impl From<&PkgDependency> for PkgTypeAndName {
    fn from(dependency: &PkgDependency) -> Self {
        PkgTypeAndName {
            pkg_type: dependency.pkg_type,
            name: dependency.name.clone(),
        }
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

pub fn get_pkg_dependencies_from_manifest_dependencies(
    manifest_dependencies: &Vec<ManifestDependency>,
) -> Result<Vec<PkgDependency>> {
    let mut dependencies = Vec::new();

    for manifest_dependency in manifest_dependencies {
        let pkg_type = PkgType::from_str(&manifest_dependency.pkg_type)?;
        let name = manifest_dependency.name.clone();
        let version_req = VersionReq::parse(&manifest_dependency.version)?;

        dependencies.push(PkgDependency {
            pkg_type,
            name,
            version_req,
            version_req_str: Some(manifest_dependency.version.clone()),
        });
    }

    Ok(dependencies)
}

/// The hash function of `Dependency`. A unique dependency is identified by its
/// type and name.
impl Hash for PkgDependency {
    fn hash<H: Hasher>(&self, state: &mut H) {
        self.pkg_type.hash(state);
        self.name.hash(state);
    }
}

impl PartialEq for PkgDependency {
    fn eq(&self, other: &Self) -> bool {
        self.pkg_type == other.pkg_type && self.name == other.name
    }
}

impl PkgDependency {
    pub fn new(
        pkg_type: PkgType,
        name: String,
        version_req: VersionReq,
    ) -> Self {
        PkgDependency {
            pkg_type,
            name,
            version_req,
            version_req_str: None,
        }
    }
}

impl Eq for PkgDependency {}

#[derive(Debug)]
pub struct DependencyRelationship {
    pub pkg_type: PkgType,
    pub name: String,
    pub version: Version,
    pub dependency: PkgDependency,
}

pub fn get_manifest_dependencies_from_pkg(
    pkg_dependencies: Vec<PkgDependency>,
) -> Vec<ManifestDependency> {
    pkg_dependencies.into_iter().map(|v| v.into()).collect()
}
