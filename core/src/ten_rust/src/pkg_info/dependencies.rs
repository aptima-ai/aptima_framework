//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
use std::{
    hash::{Hash, Hasher},
    str::FromStr,
};

use anyhow::Result;
use semver::{Version, VersionReq};
use serde::{Deserialize, Serialize};

use super::{pkg_identity::PkgIdentity, pkg_type::PkgType};
use crate::pkg_info::manifest::{dependency::ManifestDependency, Manifest};

#[derive(Clone, Debug, Serialize, Deserialize)]
pub struct PkgDependency {
    pub pkg_identity: PkgIdentity,
    pub version_req: VersionReq,
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
            pkg_identity: PkgIdentity { pkg_type, name },
            version_req,
        });
    }

    Ok(dependencies)
}

/// The hash function of `Dependency`. A unique dependency is identified by its
/// type and name.
impl Hash for PkgDependency {
    fn hash<H: Hasher>(&self, state: &mut H) {
        self.pkg_identity.hash(state);
    }
}

impl PartialEq for PkgDependency {
    fn eq(&self, other: &Self) -> bool {
        self.pkg_identity == other.pkg_identity
    }
}

impl PkgDependency {
    pub fn new(
        pkg_type: PkgType,
        name: String,
        version_req: VersionReq,
    ) -> Self {
        PkgDependency {
            pkg_identity: PkgIdentity { pkg_type, name },
            version_req,
        }
    }
}

impl Eq for PkgDependency {}

#[derive(Debug)]
pub struct DependencyRelationship {
    pub pkg_identity: PkgIdentity,
    pub version: Version,
    pub dependency: PkgDependency,
}

pub fn get_manifest_dependencies_from_pkg(
    pkg_dependencies: Vec<PkgDependency>,
) -> Vec<ManifestDependency> {
    pkg_dependencies.into_iter().map(|v| v.into()).collect()
}
