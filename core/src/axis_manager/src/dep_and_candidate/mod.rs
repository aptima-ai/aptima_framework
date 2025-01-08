//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
use std::collections::{HashMap, HashSet};
use std::path::Path;

use anyhow::{anyhow, Context, Result};
use semver::{Version, VersionReq};

use axis_rust::pkg_info::dependencies::PkgDependency;
use axis_rust::pkg_info::pkg_basic_info::PkgBasicInfo;
use axis_rust::pkg_info::pkg_type::PkgType;
use axis_rust::pkg_info::pkg_type_and_name::PkgTypeAndName;
use axis_rust::pkg_info::supports::{
    is_pkg_supports_compatible_with, PkgSupport,
};
use axis_rust::pkg_info::{get_pkg_info_from_path, PkgInfo};

use super::config::TmanConfig;
use super::fs::pathbuf_to_string;
use super::registry::{get_package_list, SearchCriteria};
use crate::log::tman_verbose_println;

// TODO(Wei): Should use the union of the semantic versioning rather than the
// union of all version requirements.
#[derive(Default)]
struct MergedVersionReq {
    version_reqs: HashSet<VersionReq>,
}

impl MergedVersionReq {
    fn new(version_req: &VersionReq) -> Self {
        MergedVersionReq {
            version_reqs: HashSet::from([version_req.clone()]),
        }
    }

    /// If there is already the same version requirement, it means there is no
    /// new information, so return false. Otherwise, return true to let the
    /// caller know that there has been a change.
    fn merge(&mut self, version_req: &VersionReq) -> Result<bool> {
        if self.version_reqs.contains(version_req) {
            return Ok(false);
        }

        self.version_reqs.insert(version_req.clone());

        Ok(true)
    }
}

/// If the contents of merged_dependencies are the same before and after the
/// merge, it means merged_dependencies has not changed, so return false.
/// Otherwise, it means there has been a change, so return true. This allows the
/// caller to know that there may be new content and that some actions may need
/// to be taken.
fn merge_dependency_to_dependencies(
    merged_dependencies: &mut HashMap<PkgTypeAndName, MergedVersionReq>,
    dependency: &PkgDependency,
) -> Result<bool> {
    let searched_target = PkgTypeAndName {
        pkg_type: dependency.type_and_name.pkg_type,
        name: dependency.type_and_name.name.clone(),
    };

    let mut changed = true;

    if let Some(existed_dependency) =
        merged_dependencies.get_mut(&searched_target)
    {
        changed = existed_dependency.merge(&dependency.version_req)?;
    } else {
        // This is the first time seeing this dependency.
        merged_dependencies.insert(
            PkgTypeAndName {
                pkg_type: dependency.type_and_name.pkg_type,
                name: dependency.type_and_name.name.clone(),
            },
            MergedVersionReq::new(&dependency.version_req),
        );
    }

    Ok(changed)
}

fn process_local_dependency_to_get_candidate(
    dependency: &PkgDependency,
    all_candidates: &mut HashMap<
        PkgTypeAndName,
        HashMap<PkgBasicInfo, PkgInfo>,
    >,
) -> Result<()> {
    // Enforce having only one candidate: the package info parsed from
    // the specified path.

    assert!(dependency.path.is_some(), "Should not happen.");

    // Construct a `PkgInfo` to represent the package corresponding to
    // the specified path.
    let base_dir = dependency.base_dir.as_deref().unwrap_or("");
    let path = dependency.path.as_deref().unwrap();

    let abs_path =
        Path::new(base_dir)
            .join(path)
            .canonicalize()
            .with_context(|| {
                format!("Failed to canonicalize path: {} + {}", base_dir, path)
            })?;

    let mut pkg_info = get_pkg_info_from_path(&abs_path, false)?;

    pkg_info.is_local_dependency = true;
    pkg_info.local_dependency_path = dependency.path.clone();
    pkg_info.local_dependency_base_dir = dependency.base_dir.clone();

    let candidate_map = all_candidates.entry((&pkg_info).into()).or_default();

    candidate_map.insert((&pkg_info).into(), pkg_info);

    Ok(())
}

async fn process_non_local_dependency_to_get_candidate(
    tman_config: &TmanConfig,
    support: &PkgSupport,
    dependency: &PkgDependency,
    all_compatible_installed_pkgs: &HashMap<
        PkgTypeAndName,
        HashMap<PkgBasicInfo, PkgInfo>,
    >,
    all_candidates: &mut HashMap<
        PkgTypeAndName,
        HashMap<PkgBasicInfo, PkgInfo>,
    >,
    new_pkgs_to_be_searched: &mut Vec<PkgInfo>,
) -> Result<()> {
    // With the current design, if there is new information, it will
    // definitely be only this version requirement. Although there may
    // be some overlap with previous version requirements,
    // given the current design, this is the best we can do
    // for now. The answer won't be wrong, but the
    // efficiency might be somewhat lower.
    let criteria = SearchCriteria {
        version_req: dependency.version_req.clone(),
    };

    // Retrieve all packages from the registry that meet the specified
    // criteria.
    let results = get_package_list(
        tman_config,
        dependency.type_and_name.pkg_type,
        &dependency.type_and_name.name,
        &criteria,
    )
    .await?;

    let mut candidate_pkg_infos: Vec<PkgInfo> = vec![];

    // Put all the results returned by the registry into
    // `candidate_pkg_infos`, making it convenient for `supports`
    // filtering later.
    for result in results {
        let mut candidate_pkg_info: PkgInfo =
            (&result.pkg_registry_info).into();

        candidate_pkg_info.url = pathbuf_to_string(result.url)?;
        candidate_pkg_info.is_installed = false;

        tman_verbose_println!(
            tman_config,
            "Collect candidate: {:?}",
            candidate_pkg_info
        );

        candidate_pkg_infos.push(candidate_pkg_info);
    }

    // Find packages from the installed packages that meet the specified
    // criteria and add them to the candidate_pkg_infos. This action ensures
    // that the dependencies within those packages are processed.
    if let Some(candidates) =
        all_compatible_installed_pkgs.get(&dependency.into())
    {
        for candidate in candidates {
            if dependency.version_req.matches(&candidate.0.version) {
                tman_verbose_println!(
                    tman_config,
                    "Collect candidate from an already installed package: {:?}",
                    candidate
                );

                candidate_pkg_infos.push(candidate.1.clone());
            }
        }
    }

    // Filter suitable candidate packages according to `supports`.
    for mut candidate_pkg_info in candidate_pkg_infos {
        tman_verbose_println!(
            tman_config,
            "Check candidate support: {:?}",
            candidate_pkg_info
        );

        let compatible_score = is_pkg_supports_compatible_with(
            &candidate_pkg_info.basic_info.supports,
            support,
        );

        // A package is considered a candidate only when compatible_score >= 0.
        if compatible_score >= 0 {
            // Record the package's compatible_score so that we can later select
            // the most appropriate one.
            candidate_pkg_info.compatible_score = compatible_score;

            tman_verbose_println!(
                tman_config,
                "Found a candidate: {:?}",
                candidate_pkg_info
            );

            all_candidates.entry(dependency.into()).or_default().insert(
                (&candidate_pkg_info).into(),
                candidate_pkg_info.clone(),
            );

            new_pkgs_to_be_searched.push(candidate_pkg_info);
        }
    }

    Ok(())
}

/// Asynchronously processes dependencies to get candidate packages.
///
/// # Parameters
///
/// - `all_candidates`: A mutable reference to a map where the keys are package
///   type & name and the values are sets of candidate package information. This
///   map will be updated with potential candidate packages for each dependency.
///
/// # Returns
///
/// A `Result` indicating the success or failure of the operation.
///
/// # Errors
///
/// This function may return an error if there is an issue processing the
/// dependencies.
async fn process_dependencies_to_get_candidates(
    tman_config: &TmanConfig,
    support: &PkgSupport,
    input_dependencies: &Vec<PkgDependency>,
    merged_dependencies: &mut HashMap<PkgTypeAndName, MergedVersionReq>,
    all_compatible_installed_pkgs: &HashMap<
        PkgTypeAndName,
        HashMap<PkgBasicInfo, PkgInfo>,
    >,
    all_candidates: &mut HashMap<
        PkgTypeAndName,
        HashMap<PkgBasicInfo, PkgInfo>,
    >,
    new_pkgs_to_be_searched: &mut Vec<PkgInfo>,
) -> Result<()> {
    for dependency in input_dependencies {
        if dependency.is_local() {
            process_local_dependency_to_get_candidate(
                dependency,
                all_candidates,
            )?;
        } else {
            // Check if we need to get the package info from the slow path.
            let changed = merge_dependency_to_dependencies(
                merged_dependencies,
                dependency,
            )?;
            if !changed {
                // There is no new information, so we won't proceed further.
                continue;
            }

            process_non_local_dependency_to_get_candidate(
                tman_config,
                support,
                dependency,
                all_compatible_installed_pkgs,
                all_candidates,
                new_pkgs_to_be_searched,
            )
            .await?;
        }
    }

    Ok(())
}

/// For a version of a package, only keep the most suitable one. The most
/// suitable one is the package with the highest compatible_score. If there are
/// multiple packages with the highest score, just pick one at random.
fn clean_up_all_candidates(
    all_candidates: &mut HashMap<
        PkgTypeAndName,
        HashMap<PkgBasicInfo, PkgInfo>,
    >,
    locked_pkgs: Option<&HashMap<PkgTypeAndName, PkgInfo>>,
) {
    for (pkg_type_name, pkg_infos) in all_candidates.iter_mut() {
        let mut version_map: HashMap<Version, &PkgInfo> = HashMap::new();
        let mut locked_pkgs_map: HashMap<Version, &PkgInfo> = HashMap::new();

        let locked_pkg =
            locked_pkgs.and_then(|locked_pkgs| locked_pkgs.get(pkg_type_name));

        for pkg_info in pkg_infos.iter() {
            // Check if the candidate is a locked one.
            if let Some(locked_pkg) = locked_pkg {
                if locked_pkg.basic_info.version
                    == pkg_info.1.basic_info.version
                    && locked_pkg.hash == pkg_info.1.hash
                {
                    locked_pkgs_map.insert(
                        pkg_info.1.basic_info.version.clone(),
                        pkg_info.1,
                    );
                }
            }

            version_map
                .entry(pkg_info.1.basic_info.version.clone())
                .and_modify(|existing_pkg_info| {
                    if pkg_info.1.compatible_score
                        > existing_pkg_info.compatible_score
                    {
                        *existing_pkg_info = pkg_info.1;
                    }
                })
                .or_insert(pkg_info.1);
        }

        // If there is a locked one, replace the candidate with the highest
        // score with the locked one.
        version_map.extend(locked_pkgs_map);

        *pkg_infos = version_map
            .into_values()
            .map(|pkg_info| (pkg_info.into(), pkg_info.clone()))
            .collect();
    }
}

pub async fn get_all_candidates_from_deps(
    tman_config: &TmanConfig,
    support: &PkgSupport,
    mut pkgs_to_be_searched: Vec<PkgInfo>,
    extra_dep: Option<&PkgDependency>,
    all_compatible_installed_pkgs: &HashMap<
        PkgTypeAndName,
        HashMap<PkgBasicInfo, PkgInfo>,
    >,
    mut all_candidates: HashMap<PkgTypeAndName, HashMap<PkgBasicInfo, PkgInfo>>,
    locked_pkgs: Option<&HashMap<PkgTypeAndName, PkgInfo>>,
) -> Result<HashMap<PkgTypeAndName, HashMap<PkgBasicInfo, PkgInfo>>> {
    let mut merged_dependencies = HashMap::new();
    let mut processed_pkgs = HashSet::<PkgBasicInfo>::new();

    // If there is extra dependencies (ex: specified in the command line),
    // handle those dependencies, too.
    if extra_dep.is_some() {
        let mut new_pkgs_to_be_searched: Vec<PkgInfo> = vec![];

        process_dependencies_to_get_candidates(
            tman_config,
            support,
            &vec![extra_dep.unwrap().clone()],
            &mut merged_dependencies,
            all_compatible_installed_pkgs,
            &mut all_candidates,
            &mut new_pkgs_to_be_searched,
        )
        .await?;

        pkgs_to_be_searched.extend(new_pkgs_to_be_searched);
    }

    loop {
        let mut new_pkgs_to_be_searched: Vec<PkgInfo> = vec![];

        // Process all packages to be searched.
        while let Some(pkg_to_be_search) = pkgs_to_be_searched.pop() {
            if processed_pkgs.contains(&(&pkg_to_be_search).into()) {
                // If this package info has already been processed, do not
                // process it again.
                continue;
            }

            process_dependencies_to_get_candidates(
                tman_config,
                support,
                &pkg_to_be_search.dependencies,
                &mut merged_dependencies,
                all_compatible_installed_pkgs,
                &mut all_candidates,
                &mut new_pkgs_to_be_searched,
            )
            .await?;

            // Remember that this package has already been processed.
            processed_pkgs.insert((&pkg_to_be_search).into());
        }

        if new_pkgs_to_be_searched.is_empty() {
            break;
        }
        pkgs_to_be_searched.extend(new_pkgs_to_be_searched);
    }

    clean_up_all_candidates(&mut all_candidates, locked_pkgs);

    Ok(all_candidates)
}

pub fn get_pkg_info_from_candidates(
    pkg_type: &str,
    pkg_name: &str,
    version: &str,
    all_candidates: &HashMap<PkgTypeAndName, HashMap<PkgBasicInfo, PkgInfo>>,
) -> Result<PkgInfo> {
    let pkg_type_name = PkgTypeAndName {
        pkg_type: pkg_type.parse::<PkgType>()?,
        name: pkg_name.to_string(),
    };
    let version_parsed = Version::parse(version)?;
    let pkg_info = all_candidates
        .get(&pkg_type_name)
        .and_then(|set| {
            set.iter()
                .find(|pkg| pkg.1.basic_info.version == version_parsed)
        })
        .ok_or_else(|| {
            anyhow!(
                "PkgInfo not found for [{}]{}@{}",
                pkg_type,
                pkg_name,
                version
            )
        })?;
    Ok(pkg_info.1.clone())
}
