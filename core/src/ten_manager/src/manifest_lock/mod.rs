//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
use std::{collections::HashMap, fs, path::Path, str::FromStr};

use anyhow::{Ok, Result};
use console::Emoji;
use semver::{Version, VersionReq};
use serde::{Deserialize, Serialize};

use super::constants::MANIFEST_LOCK_JSON_FILENAME;
use ten_rust::json_schema::validate_manifest_lock_json_string;
use ten_rust::pkg_info::dependencies::PkgDependency;
use ten_rust::pkg_info::manifest::support::ManifestSupport;
use ten_rust::pkg_info::supports::{
    get_manifest_supports_from_pkg, get_pkg_supports_from_manifest_supports,
};
use ten_rust::pkg_info::{
    pkg_identity::PkgIdentity, pkg_type::PkgType, PkgInfo,
};

// The `manifest-lock.json` structure.
#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct ManifestLock {
    #[serde(skip_serializing_if = "Option::is_none")]
    pub version: Option<u32>,

    #[serde(skip_serializing_if = "Option::is_none")]
    pub packages: Option<Vec<ManifestLockItem>>,
}

type LockedPkgsInfo<'a> = &'a Vec<&'a PkgInfo>;

impl<'a> From<LockedPkgsInfo<'a>> for ManifestLock {
    // Convert a complete `Resolve` to a ManifestLock which can be serialized to
    // a `manifest-lock.json` file.
    fn from(resolve: LockedPkgsInfo) -> Self {
        ManifestLock {
            version: Some(1), // Not used for now.
            packages: Some(
                resolve
                    .iter()
                    .map(|pkg_info| ManifestLockItem::from(pkg_info.to_owned()))
                    .collect(),
            ),
        }
    }
}

impl FromStr for ManifestLock {
    type Err = anyhow::Error;

    fn from_str(s: &str) -> Result<Self, Self::Err> {
        validate_manifest_lock_json_string(s)?;
        let manifest_lock: ManifestLock = serde_json::from_str(s)?;

        Ok(manifest_lock)
    }
}

impl ManifestLock {
    pub fn get_pkgs(&self) -> HashMap<PkgIdentity, PkgInfo> {
        self.packages
            .as_ref()
            .map(|pkgs| {
                pkgs.iter()
                    .map(|pkg| {
                        let pkg_info: PkgInfo = pkg.into();
                        (pkg_info.pkg_identity.clone(), pkg_info)
                    })
                    .collect()
            })
            .unwrap_or_default()
    }

    pub fn print_changes(&self, old_resolve: &ManifestLock) {
        let old_pkgs = old_resolve.get_pkgs();
        let new_pkgs = self.get_pkgs();

        let mut added_pkgs = vec![];
        let mut removed_pkgs = vec![];
        let mut updated_pkgs = vec![];

        for (idt, old_pkg) in old_pkgs.iter() {
            let contains = new_pkgs.contains_key(idt);
            if !contains {
                removed_pkgs.push(old_pkg);
            } else {
                let new_pkg = new_pkgs.get(idt).unwrap();
                if old_pkg.version != new_pkg.version {
                    updated_pkgs.push((old_pkg, new_pkg));
                }
            }
        }

        for (idt, new_pkg) in new_pkgs.iter() {
            let contains = old_pkgs.contains_key(idt);
            if !contains {
                added_pkgs.push(new_pkg);
            }
        }

        if !added_pkgs.is_empty() {
            for pkg in added_pkgs.iter() {
                println!(
                    "{}  Adding package {} v{}",
                    Emoji("🔒", ""),
                    pkg.pkg_identity.name,
                    pkg.version
                );
            }
        }

        if !removed_pkgs.is_empty() {
            for pkg in removed_pkgs.iter() {
                println!(
                    "{}  Removing package {} v{}",
                    Emoji("🔒", ""),
                    pkg.pkg_identity.name,
                    pkg.version
                );
            }
        }

        if !updated_pkgs.is_empty() {
            for (old_pkg, new_pkg) in updated_pkgs.iter() {
                println!(
                    "{}  Updating package {} v{} to v{}",
                    Emoji("🔒", ""),
                    old_pkg.pkg_identity.name,
                    old_pkg.version,
                    new_pkg.version
                );
            }
        }
    }
}

fn are_equal_lockfiles(lock_file_path: &Path, resolve_str: &str) -> bool {
    // Read the contents of the lock file.
    let lock_file_str = crate::utils::read_file_to_string(lock_file_path)
        .unwrap_or_else(|_| "".to_string());

    // Compare the lock file contents with the new resolve string.
    lock_file_str.lines().eq(resolve_str.lines())
}

// Serialize the `ManifestLock` to a JSON string and write it to the lock file.
pub fn write_pkg_lockfile<P: AsRef<Path>>(
    manifest_lock: &ManifestLock,
    app_path: P,
) -> Result<bool> {
    // Serialize the `ManifestLock` to a JSON string.
    let encodable_resolve_str = serde_json::to_string_pretty(manifest_lock)?;

    let lock_file_path = app_path.as_ref().join(MANIFEST_LOCK_JSON_FILENAME);

    // If the lock file contents haven't changed, we don't need to rewrite it.
    if are_equal_lockfiles(lock_file_path.as_ref(), &encodable_resolve_str) {
        return Ok(false);
    }

    // TODO(xilin): Maybe RWlock is needed.
    fs::write(lock_file_path, encodable_resolve_str)?;
    Ok(true)
}

fn parse_manifest_lock_from_file<P: AsRef<Path>>(
    manifest_lock_file_path: P,
) -> Result<ManifestLock> {
    // Read the contents of the manifest-lock.json file.
    let content = crate::utils::read_file_to_string(manifest_lock_file_path)?;

    ManifestLock::from_str(&content)
}

pub fn parse_manifest_lock_in_folder(
    folder_path: &Path,
) -> Result<ManifestLock> {
    // Path to the manifest-lock.json file.
    let manifest_lock_file_path = folder_path.join(MANIFEST_LOCK_JSON_FILENAME);

    // Read and parse the manifest-lock.json file.
    let manifest_lock =
        parse_manifest_lock_from_file(manifest_lock_file_path.clone())?;

    Ok(manifest_lock)
}

// The `dependencies` field structure in `manifest-lock.json`.
#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct ManifestLockItem {
    #[serde(rename = "type")]
    pub pkg_type: String,

    pub name: String,
    pub version: String,
    pub hash: String,

    #[serde(skip_serializing_if = "Option::is_none")]
    pub dependencies: Option<Vec<ManifestLockItemDependencyItem>>,

    #[serde(skip_serializing_if = "Option::is_none")]
    pub supports: Option<Vec<ManifestSupport>>,
}

fn get_encodable_deps_from_pkg_deps(
    pkg_deps: Vec<PkgDependency>,
) -> Vec<ManifestLockItemDependencyItem> {
    pkg_deps.into_iter().map(|dep| dep.into()).collect()
}

impl<'a> From<&'a PkgInfo> for ManifestLockItem {
    fn from(pkg_info: &'a PkgInfo) -> Self {
        ManifestLockItem {
            pkg_type: pkg_info.pkg_identity.pkg_type.to_string(),
            name: pkg_info.pkg_identity.name.clone(),
            version: pkg_info.version.to_string(),
            hash: pkg_info.hash.to_string(),
            dependencies: if pkg_info.dependencies.is_empty() {
                None
            } else {
                Some(get_encodable_deps_from_pkg_deps(
                    pkg_info.dependencies.clone(),
                ))
            },
            supports: Some(get_manifest_supports_from_pkg(&pkg_info.supports)),
        }
    }
}

impl<'a> From<&'a ManifestLockItem> for PkgInfo {
    fn from(val: &'a ManifestLockItem) -> Self {
        PkgInfo {
            pkg_identity: PkgIdentity {
                pkg_type: PkgType::from_str(&val.pkg_type).unwrap(),
                name: val.name.clone(),
            },
            version: Version::parse(&val.version).unwrap(),
            dependencies: val
                .clone()
                .dependencies
                .map(|deps| {
                    deps.into_iter()
                        .map(|dep| PkgDependency {
                            pkg_identity: PkgIdentity {
                                pkg_type: PkgType::from_str(&dep.pkg_type)
                                    .unwrap(),
                                name: dep.name,
                            },
                            version_req: VersionReq::STAR,
                        })
                        .collect()
                })
                .unwrap_or_default(),
            api: None,
            // If val.supports is None, then supports is an empty vector.
            // Otherwise, convert the supports to a vector of PkgSupport.
            supports: get_pkg_supports_from_manifest_supports(&val.supports)
                .unwrap(),
            compatible_score: 0, // TODO(xilin): default value.
            is_local_installed: false,
            url: "".to_string(), // TODO(xilin): default value.
            hash: val.hash.clone(),
            manifest: None,
            property: None,
            predefined_graphs: vec![],
            schema_store: None,
        }
    }
}

#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct ManifestLockItemDependencyItem {
    #[serde(rename = "type")]
    pub pkg_type: String,

    pub name: String,
}

impl From<PkgDependency> for ManifestLockItemDependencyItem {
    fn from(pkg_dep: PkgDependency) -> Self {
        ManifestLockItemDependencyItem {
            pkg_type: pkg_dep.pkg_identity.pkg_type.to_string(),
            name: pkg_dep.pkg_identity.name,
        }
    }
}
