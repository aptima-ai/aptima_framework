//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
pub mod api;
mod binding;
pub mod constants;
pub mod dependencies;
pub mod graph;
pub mod hash;
pub mod language;
pub mod manifest;
pub mod message;
pub mod pkg_basic_info;
pub mod pkg_type;
pub mod pkg_type_and_name;
pub mod predefined_graphs;
pub mod property;
pub mod supports;
mod utils;
pub mod value_type;

use std::{
    collections::HashMap,
    path::{Path, PathBuf},
};

use anyhow::Result;
use graph::Graph;
use pkg_basic_info::PkgBasicInfo;
use pkg_type_and_name::PkgTypeAndName;

use crate::schema::store::SchemaStore;
use api::PkgApi;
use constants::{
    ADDON_LOADER_DIR, ERR_STR_NOT_APP_DIR, EXTENSION_DIR,
    MANIFEST_JSON_FILENAME, PROTOCOL_DIR, SYSTEM_DIR, TEN_PACKAGES_DIR,
};
use dependencies::{get_pkg_dependencies_from_manifest, PkgDependency};
use manifest::{parse_manifest_from_file, parse_manifest_in_folder, Manifest};
use pkg_type::PkgType;
use property::{
    parse_property_from_file, parse_property_in_folder,
    predefined_graph::PredefinedGraph, Property,
};

pub fn localhost() -> String {
    "localhost".to_string()
}

#[derive(Clone, Debug)]
pub struct PkgInfo {
    pub basic_info: PkgBasicInfo,

    pub dependencies: Vec<PkgDependency>,
    pub api: Option<PkgApi>,

    pub compatible_score: i32,

    /// This indicates that the package has been installed in the correct
    /// location. For example, in the case of an extension, it means it has
    /// been installed under the `ten_packages/` directory.
    pub is_installed: bool,

    pub url: String,
    pub hash: String,

    pub manifest: Option<Manifest>,
    pub property: Option<Property>,

    pub schema_store: Option<SchemaStore>,

    /// Indicates that the `pkg_info` represents a local dependency package.
    pub is_local_dependency: bool,
    pub local_dependency_path: Option<String>,
}

impl PkgInfo {
    pub fn from_metadata(
        manifest: &Manifest,
        property: &Option<Property>,
    ) -> Result<Self> {
        let mut pkg_info = PkgInfo {
            basic_info: PkgBasicInfo::try_from(manifest)?,

            dependencies: get_pkg_dependencies_from_manifest(manifest)?,
            api: PkgApi::from_manifest(manifest)?,
            compatible_score: -1,

            is_installed: false,
            url: String::new(),
            hash: String::new(),

            manifest: Some(manifest.clone()),
            property: property.clone(),
            schema_store: SchemaStore::from_manifest(manifest)?,

            is_local_dependency: false,
            local_dependency_path: None,
        };

        pkg_info.hash = pkg_info.gen_hash_hex();

        Ok(pkg_info)
    }

    pub fn is_manifest_equal_to_fs(&self) -> Result<bool> {
        if self.url.is_empty() {
            return Ok(false);
        }

        let manifest_from_pkg = match &self.manifest {
            Some(manifest) => manifest,
            None => return Ok(false),
        };

        let manifest_json_path =
            PathBuf::from(&self.url).join(MANIFEST_JSON_FILENAME);
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

    pub fn get_predefined_graphs(&self) -> Option<&Vec<PredefinedGraph>> {
        if let Some(property) = &self.property {
            if let Some(ten) = &property._ten {
                return ten.predefined_graphs.as_ref();
            }
        }

        None
    }

    pub fn update_predefined_graph(&mut self, new_graph: &PredefinedGraph) {
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

    pub fn get_dependency_by_type_and_name(
        &self,
        pkg_type: &str,
        pkg_name: &str,
    ) -> Option<&PkgDependency> {
        self.dependencies.iter().find(|dep| {
            dep.type_and_name.pkg_type.to_string() == pkg_type
                && dep.type_and_name.name == pkg_name
        })
    }
}

pub fn get_pkg_info_from_path(
    pkg_path: &Path,
    is_installed: bool,
) -> Result<PkgInfo> {
    let manifest = parse_manifest_in_folder(pkg_path)?;
    let property = parse_property_in_folder(pkg_path)?;

    let mut pkg_info: PkgInfo = PkgInfo::from_metadata(&manifest, &property)?;

    pkg_info.is_installed = is_installed;
    pkg_info.url = pkg_path.to_string_lossy().to_string();

    Ok(pkg_info)
}

fn collect_pkg_info_from_path(
    base_path: &Path,
    pkgs_info: &mut HashMap<PkgTypeAndName, PkgInfo>,
) -> Result<Manifest> {
    let pkg_info = get_pkg_info_from_path(base_path, true)?;

    let pkg_type_name = PkgTypeAndName::from(&pkg_info);
    if pkgs_info.contains_key(&pkg_type_name) {
        return Err(anyhow::anyhow!(
            "Duplicated package, type: {}, name: {}",
            pkg_info.basic_info.type_and_name.pkg_type,
            pkg_info.basic_info.type_and_name.name
        ));
    }

    let manifest = pkg_info.manifest.clone().unwrap().clone();

    pkgs_info.insert(pkg_type_name, pkg_info);

    Ok(manifest)
}

pub fn get_all_existed_pkgs_info_of_app_to_hashmap(
    app_path: &Path,
) -> Result<HashMap<PkgTypeAndName, PkgInfo>> {
    let mut pkgs_info: HashMap<PkgTypeAndName, PkgInfo> = HashMap::new();

    // Process the manifest.json file in the root path.
    let app_pkg_manifest =
        collect_pkg_info_from_path(app_path, &mut pkgs_info)?;
    if app_pkg_manifest.type_and_name.pkg_type != PkgType::App {
        return Err(anyhow::anyhow!(ERR_STR_NOT_APP_DIR));
    }

    // Define paths to include manifest.json files from.
    let addon_type_dirs =
        vec![EXTENSION_DIR, PROTOCOL_DIR, ADDON_LOADER_DIR, SYSTEM_DIR];

    for addon_type_dir in addon_type_dirs {
        let allowed_path = app_path.join(TEN_PACKAGES_DIR).join(addon_type_dir);

        if allowed_path.exists() && allowed_path.is_dir() {
            for entry in allowed_path.read_dir()?.flatten() {
                let path = entry.path();

                if path.is_dir() {
                    let manifest_path = path.join(MANIFEST_JSON_FILENAME);

                    if manifest_path.exists() && manifest_path.is_file() {
                        let manifest =
                            parse_manifest_from_file(&manifest_path)?;

                        // Do some simple checks.
                        if manifest.type_and_name.name
                            != path.file_name().unwrap().to_str().unwrap()
                        {
                            return Err(anyhow::anyhow!(
                                "The path '{}' is not valid: {}.",
                                format!("{}/{}",addon_type_dir, manifest.type_and_name.name),
                                format!(
                                    "the path '{}' and the name '{}' of the package are different",
                                    path.file_name().unwrap().to_str().unwrap(), manifest.type_and_name.name
                            )));
                        }

                        if manifest.type_and_name.pkg_type.to_string()
                            != addon_type_dir
                        {
                            return Err(anyhow::anyhow!(
                                "The path '{}' is not valid: {}.",
                                format!(
                                    "{}/{}",
                                    addon_type_dir,
                                    manifest.type_and_name.name
                                ),
                                format!(
                                "the package type '{}' is not as expected '{}'",
                                manifest.type_and_name.pkg_type.to_string(), addon_type_dir)
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
    let result = get_all_existed_pkgs_info_of_app_to_hashmap(app_path)?;
    Ok(result.into_values().collect())
}

pub fn find_untracked_local_packages<'a>(
    dependencies: &[&'a PkgInfo],
    local_pkgs: &[&'a PkgInfo],
) -> Vec<&'a PkgInfo> {
    let mut untracked_local_packages: Vec<&PkgInfo> = vec![];

    for local_pkg in local_pkgs {
        if local_pkg.basic_info.type_and_name.pkg_type == PkgType::App {
            continue;
        }

        // Check if the package is in dependencies list.
        if !dependencies.iter().any(|dependency| {
            dependency.basic_info.type_and_name.pkg_type
                == local_pkg.basic_info.type_and_name.pkg_type
                && dependency.basic_info.type_and_name.name
                    == local_pkg.basic_info.type_and_name.name
        }) {
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
        let pkg_in_dependencies = dependencies.iter().find(|pkg| {
            pkg.basic_info.type_and_name.pkg_type
                == local_pkg.basic_info.type_and_name.pkg_type
                && pkg.basic_info.type_and_name.name
                    == local_pkg.basic_info.type_and_name.name
        });

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
            if !pkg_in_dependencies.is_installed {
                result.push((pkg_in_dependencies, local_pkg));
            }
        }
    }

    result
}

/// Check the graph for current app.
///
/// # Arguments
/// * `app_base_dir` - The absolute path of the app base directory, required.
/// * `graph_json` - The graph definition in JSON format, required.
/// * `app_uri` - The `_ten::uri` of the app, required. We do not read this
///   content from property.json of app, because the property.json might not
///   exist. Ex: the property of app might be customized using
///   `init_property_from_json` in `on_configure` method.
pub fn ten_rust_check_graph_for_app(
    app_base_dir: &str,
    graph_json: &str,
    app_uri: &str,
) -> Result<()> {
    let app_path = Path::new(app_base_dir);
    if !app_path.exists() {
        return Err(anyhow::anyhow!(
            "The app base dir [{}] is not found.",
            app_base_dir
        ));
    }

    let mut pkgs_of_app: HashMap<String, Vec<PkgInfo>> = HashMap::new();
    let pkgs_info = get_all_existed_pkgs_info_of_app(app_path)?;
    pkgs_of_app.insert(app_uri.to_string(), pkgs_info);

    // `Graph::from_str` calls `validate`, and `validate` checks that there are
    // no `localhost` entries in the graph JSON (as per our rule). However, the
    // TEN runtime first processes the content of the graph JSON, inserting
    // the appropriate `localhost` string before passing it to the Rust
    // side. Therefore, the graph JSON received here might already includes the
    // `localhost` string processed by the TEN runtime, so `Graph::from_str`
    // cannot be used in this context.
    //
    // let graph = Graph::from_str(graph_json)?;
    let graph: Graph = serde_json::from_str(graph_json)?;
    graph.check_for_single_app(&pkgs_of_app)
}
