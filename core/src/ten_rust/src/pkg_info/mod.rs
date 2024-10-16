//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
pub mod api;
mod constants;
pub mod dependencies;
pub mod graph;
pub mod hash;
pub mod language;
pub mod manifest;
pub mod message;
pub mod pkg_identity;
pub mod pkg_type;
pub mod predefined_graphs;
pub mod property;
pub mod supports;
mod utils;
pub mod value_type;

use std::{
    cmp::Ordering,
    collections::HashSet,
    hash::{Hash, Hasher},
    path::{Path, PathBuf},
};

use anyhow::Result;
use semver::Version;

use crate::schema::store::SchemaStore;
use api::PkgApi;
use constants::{
    APP_PKG_TYPE, ERR_STR_NOT_APP_DIR, EXTENSION_DIR, EXTENSION_GROUP_DIR,
    MANIFEST_JSON_FILENAME, PROTOCOL_DIR, SYSTEM_DIR, TEN_PACKAGES_DIR,
};
use dependencies::{get_pkg_dependencies_from_manifest, PkgDependency};
use manifest::{parse_manifest_from_file, parse_manifest_in_folder, Manifest};
use pkg_identity::PkgIdentity;
use pkg_type::PkgType;
use property::{
    parse_property_from_file, parse_property_in_folder,
    predefined_graph::PropertyPredefinedGraph, Property,
};
use supports::{get_pkg_supports_from_manifest, PkgSupport};

pub fn localhost() -> String {
    "localhost".to_string()
}

#[derive(Clone, Debug)]
pub struct PkgInfo {
    pub pkg_identity: PkgIdentity,
    pub version: Version,
    pub dependencies: Vec<PkgDependency>,
    pub api: Option<PkgApi>,

    // Since the declaration 'does not support all environments' has no
    // practical meaning, not specifying the 'supports' field or specifying an
    // empty 'supports' field both represent support for all environments.
    // Therefore, the 'supports' field here is not an option. An empty
    // 'supports' field represents support for all combinations of
    // environments.
    pub supports: Vec<PkgSupport>,
    pub compatible_score: i32,

    // Source information.
    pub is_local_installed: bool,
    pub url: String,
    pub hash: String,

    pub manifest: Option<Manifest>,
    pub property: Option<Property>,

    pub schema_store: Option<SchemaStore>,
}

// Two PkgInfo(s) are equal if their package identify (type & name), version and
// supports are the same.
impl Hash for PkgInfo {
    fn hash<H: Hasher>(&self, state: &mut H) {
        self.pkg_identity.hash(state);
        self.version.hash(state);
        self.supports.hash(state);
    }
}

impl PartialEq for PkgInfo {
    fn eq(&self, other: &Self) -> bool {
        self.pkg_identity == other.pkg_identity && self.version == other.version
    }
}

impl Eq for PkgInfo {}

impl PartialOrd for PkgInfo {
    fn partial_cmp(&self, other: &Self) -> Option<Ordering> {
        Some(self.cmp(other))
    }
}

impl Ord for PkgInfo {
    fn cmp(&self, other: &Self) -> Ordering {
        if self.pkg_identity.pkg_type != other.pkg_identity.pkg_type {
            return self
                .pkg_identity
                .pkg_type
                .cmp(&other.pkg_identity.pkg_type);
        }

        if self.pkg_identity.name != other.pkg_identity.name {
            self.pkg_identity.name.cmp(&other.pkg_identity.name)
        } else {
            self.version.cmp(&other.version)
        }
    }
}

impl PkgInfo {
    pub fn from_metadata(
        manifest: &Manifest,
        property: &Option<Property>,
    ) -> Result<Self> {
        Ok(PkgInfo {
            pkg_identity: PkgIdentity::from_manifest(manifest)?,
            version: Version::parse(&manifest.version)?,
            dependencies: get_pkg_dependencies_from_manifest(manifest)?,
            api: PkgApi::from_manifest(manifest)?,
            supports: get_pkg_supports_from_manifest(manifest)?,
            compatible_score: -1,

            is_local_installed: false,
            url: "".to_string(),
            hash: manifest.gen_hash_hex()?,

            manifest: Some(manifest.clone()),
            property: property.clone(),
            schema_store: SchemaStore::from_manifest(manifest)?,
        })
    }

    pub fn is_manifest_equal_to_fs(&self) -> Result<bool> {
        if self.url.is_empty() {
            return Ok(false);
        }

        let manifest_from_pkg = match &self.manifest {
            Some(manifest) => manifest,
            None => return Ok(false),
        };

        let manifest_json_path = PathBuf::from(&self.url).join("manifest.json");
        let manifest_from_fs: Manifest =
            parse_manifest_from_file(&manifest_json_path)?;

        // Convert to standard JSON struct.
        let manifest_pkg_json = serde_json::to_value(manifest_from_pkg)?;
        let manifest_fs_json = serde_json::to_value(manifest_from_fs)?;

        Ok(manifest_pkg_json == manifest_fs_json)
    }

    pub fn is_property_equal_to_fs(&self) -> Result<bool> {
        if self.url.is_empty() {
            return Ok(false);
        }

        let property_from_pkg = match &self.property {
            Some(property) => property,
            None => return Ok(false),
        };

        let property_json_path = PathBuf::from(&self.url).join("property.json");
        let property_from_fs: Property =
            parse_property_from_file(&property_json_path)?;

        // Convert to standard JSON struct.
        let property_pkg_json = serde_json::to_value(property_from_pkg)?;
        let property_fs_json = serde_json::to_value(property_from_fs)?;

        Ok(property_pkg_json == property_fs_json)
    }

    pub fn get_predefined_graphs(
        &self,
    ) -> Option<&Vec<PropertyPredefinedGraph>> {
        if let Some(property) = &self.property {
            if let Some(ten) = &property._ten {
                return ten.predefined_graphs.as_ref();
            }
        }

        None
    }

    pub fn update_predefined_graph(
        &mut self,
        new_graph: &PropertyPredefinedGraph,
    ) {
        if let Some(property) = &mut self.property {
            if let Some(ten) = &mut property._ten {
                if let Some(predefined_graphs) = &mut ten.predefined_graphs {
                    if let Some(old_graph) = predefined_graphs
                        .iter_mut()
                        .find(|g| g.name == new_graph.name)
                    {
                        *old_graph = new_graph.clone();
                    }
                }
            }
        }
    }
}

pub fn get_pkg_info_from_path(pkg_path: &Path) -> Result<PkgInfo> {
    let manifest = parse_manifest_in_folder(pkg_path)?;
    let property = match parse_property_in_folder(pkg_path) {
        Ok(prop) => Some(prop),
        Err(_) => None,
    };

    let mut pkg_info: PkgInfo = PkgInfo::from_metadata(&manifest, &property)?;

    pkg_info.is_local_installed = true;
    pkg_info.url = pkg_path.to_string_lossy().to_string();

    Ok(pkg_info)
}

fn collect_pkg_info_from_path(
    base_path: &Path,
    pkgs_info: &mut HashSet<PkgInfo>,
) -> Result<Manifest> {
    let pkg_info = get_pkg_info_from_path(base_path)?;

    if pkgs_info.contains(&pkg_info) {
        return Err(anyhow::anyhow!(
            "Duplicated package, type: {}, name: {}",
            pkg_info.pkg_identity.pkg_type,
            pkg_info.pkg_identity.name
        ));
    }

    let manifest = pkg_info.manifest.clone().unwrap().clone();

    pkgs_info.insert(pkg_info);

    Ok(manifest)
}

pub fn get_all_existed_pkgs_info_of_app_to_hashset(
    app_path: &Path,
) -> Result<HashSet<PkgInfo>> {
    let mut pkgs_info: HashSet<PkgInfo> = HashSet::new();

    // Process the manifest.json file in the root path.
    match collect_pkg_info_from_path(app_path, &mut pkgs_info) {
        Ok(manifest) => {
            if manifest.pkg_type != APP_PKG_TYPE {
                return Err(anyhow::anyhow!(ERR_STR_NOT_APP_DIR));
            }
            manifest
        }
        Err(_) => {
            return Err(anyhow::anyhow!(ERR_STR_NOT_APP_DIR));
        }
    };

    // Define paths to include manifest.json files from.
    let addon_types =
        vec![EXTENSION_DIR, EXTENSION_GROUP_DIR, PROTOCOL_DIR, SYSTEM_DIR];

    for addon_type in addon_types {
        let allowed_path = app_path.join(TEN_PACKAGES_DIR).join(addon_type);

        if allowed_path.exists() && allowed_path.is_dir() {
            for entry in allowed_path.read_dir()?.flatten() {
                let path = entry.path();

                if path.is_dir() {
                    let manifest_path = path.join(MANIFEST_JSON_FILENAME);

                    if manifest_path.exists() && manifest_path.is_file() {
                        let manifest =
                            parse_manifest_from_file(&manifest_path)?;

                        // Do some simple checks.
                        if manifest.name
                            != path.file_name().unwrap().to_str().unwrap()
                        {
                            return Err(anyhow::anyhow!(
                                "The path '{}' is not valid: {}.",
                                format!("{}:{}",manifest.pkg_type, manifest.name),
                                format!(
                                    "the path '{}' and the name '{}' of the package are different",
                                    path.file_name().unwrap().to_str().unwrap(), manifest.name
                            )));
                        }

                        if manifest.pkg_type != addon_type {
                            return Err(anyhow::anyhow!(
                                "The path '{}' is not valid: {}.",
                                format!(
                                    "{}:{}",
                                    manifest.pkg_type, manifest.name
                                ),
                                format!(
                                "the package type '{}' is not as expected '{}'",
                                manifest.pkg_type, addon_type)
                            ));
                        }

                        // This folder contains a manifest.json file and
                        // that manifest.json file is a correct TEN
                        // manifest.json file, and this package is a
                        // dependency of app, so read it to treat it as a
                        // local dependency.

                        collect_pkg_info_from_path(&path, &mut pkgs_info)?;
                    }
                }
            }
        }
    }

    Ok(pkgs_info)
}

pub fn get_all_existed_pkgs_info_of_app(
    app_path: &Path,
) -> Result<Vec<PkgInfo>> {
    let result = get_all_existed_pkgs_info_of_app_to_hashset(app_path)?;
    Ok(result.into_iter().collect())
}

pub fn find_untracked_local_packages<'a>(
    dependencies: &[&'a PkgInfo],
    local_pkgs: &[&'a PkgInfo],
) -> Vec<&'a PkgInfo> {
    let mut untracked_local_packages: Vec<&PkgInfo> = vec![];

    for local_pkg in local_pkgs {
        let pkg_identity = &local_pkg.pkg_identity;

        if pkg_identity.pkg_type == PkgType::App {
            continue;
        }

        // Check if the package is in dependencies list.
        if !dependencies
            .iter()
            .any(|dependency| dependency.pkg_identity == *pkg_identity)
        {
            untracked_local_packages.push(local_pkg);
        }
    }

    untracked_local_packages.into_iter().collect()
}

/**
 * Return a list of tuples, each tuple contains a local installed package
 * and the package with the same identity but not installed locally in the
 * dependencies list.
 *
 * The possible reason could be that the 'supports' is incompatible, or the
 * version calculated by the dependency tree is different.
 */
pub fn find_to_be_replaced_local_pkgs<'a>(
    dependencies: &[&'a PkgInfo],
    local_pkgs: &[&'a PkgInfo],
) -> Vec<(&'a PkgInfo, &'a PkgInfo)> {
    let mut result: Vec<(&PkgInfo, &PkgInfo)> = vec![];

    for local_pkg in local_pkgs {
        let pkg_identity = local_pkg.pkg_identity.clone();

        let pkg_in_dependencies = dependencies
            .iter()
            .find(|pkg| pkg.pkg_identity == pkg_identity);

        if let Some(pkg_in_dependencies) = pkg_in_dependencies {
            // If the supports of a locally installed package are incompatible,
            // it will not be included in the initial candidate list, so there
            // will not be any package with the same identity in the dependency
            // tree with `is_local_installed` set to `true`.
            //
            // Additionally, if the version calculated by the dependency tree
            // differs from the currently installed local version, the
            // `is_local_installed` of that package in the dependency tree will
            // also not be `true`. Therefore, we will uniformly use
            // `is_local_installed` to make overall judgments.
            if !pkg_in_dependencies.is_local_installed {
                result.push((pkg_in_dependencies, local_pkg));
            }
        }
    }

    result
}
