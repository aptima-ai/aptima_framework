//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
use std::collections::{HashMap, HashSet};
use std::hash::{Hash, Hasher};

use anyhow::{anyhow, Result};
use semver::{Version, VersionReq};

use ten_rust::pkg_info::dependencies::PkgDependency;
use ten_rust::pkg_info::pkg_identity::PkgIdentity;
use ten_rust::pkg_info::pkg_type::PkgType;
use ten_rust::pkg_info::supports::{
    is_pkg_supports_compatible_with, PkgSupport,
};
use ten_rust::pkg_info::PkgInfo;

use super::config::TmanConfig;
use super::registry::{get_package_list, SearchCriteria};
use super::utils::pathbuf_to_string;
use crate::package_info::pkg_info_from_find_package_data;

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

struct FoundDependency {
    pkg_identity: PkgIdentity,
    version_reqs: MergedVersionReq,
}

impl Hash for FoundDependency {
    fn hash<H: Hasher>(&self, state: &mut H) {
        self.pkg_identity.hash(state);
    }
}

impl PartialEq for FoundDependency {
    fn eq(&self, other: &Self) -> bool {
        self.pkg_identity == other.pkg_identity
    }
}

impl Eq for FoundDependency {}

/// If the contents of merged_dependencies are the same before and after the
/// merge, it means merged_dependencies has not changed, so return false.
/// Otherwise, it means there has been a change, so return true. This allows the
/// caller to know that there may be new content and that some actions may need
/// to be taken.
fn merge_dependency_to_dependencies(
    merged_dependencies: &mut HashSet<FoundDependency>,
    dependency: &PkgDependency,
) -> Result<bool> {
    let searched_target = FoundDependency {
        pkg_identity: dependency.pkg_identity.clone(),
        version_reqs: MergedVersionReq::default(),
    };

    let mut changed = true;

    // Note that FoundDependency will only use pkg_identity as the hash key.
    if let Some(mut existed_dependency) =
        merged_dependencies.take(&searched_target)
    {
        changed = existed_dependency
            .version_reqs
            .merge(&dependency.version_req)?;

        // Put back.
        merged_dependencies.insert(existed_dependency);
    } else {
        // This is the first time seeing this dependency.

        merged_dependencies.insert(FoundDependency {
            pkg_identity: dependency.pkg_identity.clone(),
            version_reqs: MergedVersionReq::new(&dependency.version_req),
        });
    }

    Ok(changed)
}

/// Asynchronously processes dependencies to get candidate packages.
///
/// # Parameters
///
/// - `all_candidates`: A mutable reference to a map where the keys are package
///   identities (`PkgIdentity`) and the values are sets of candidate package
///   information (`HashSet<PkgInfo>`). This map will be updated with potential
///   candidate packages for each dependency.
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
    merged_dependencies: &mut HashSet<FoundDependency>,
    all_candidates: &mut HashMap<PkgIdentity, HashSet<PkgInfo>>,
    new_pkgs_to_be_searched: &mut Vec<PkgInfo>,
) -> Result<()> {
    for dependency in input_dependencies {
        // Check if we need to get the package info from the slow path.
        let changed =
            merge_dependency_to_dependencies(merged_dependencies, dependency)?;
        if !changed {
            // There is no new information, so we won't proceed further.
            continue;
        }

        // With the current design, if there is new information, it will
        // definitely be only this version requirement. Although there may be
        // some overlap with previous version requirements, given the current
        // design, this is the best we can do for now. The answer won't be
        // wrong, but the efficiency might be somewhat lower.
        let criteria = SearchCriteria {
            version_req: dependency.version_req.clone(),
        };

        // Retrieve all packages from the registry that meet the specified
        // criteria.
        let results =
            get_package_list(tman_config, &dependency.pkg_identity, &criteria)
                .await?;

        let mut candidate_pkg_infos: Vec<PkgInfo> = vec![];

        // Put all the results returned by the registry into
        // `candidate_pkg_infos`, making it convenient for `supports` filtering
        // later.
        for result in results {
            let mut candidate_pkg_info =
                pkg_info_from_find_package_data(&result.package_data)?;

            candidate_pkg_info.url = pathbuf_to_string(result.url)?;
            candidate_pkg_info.is_local_installed = false;

            candidate_pkg_infos.push(candidate_pkg_info);
        }

        // Find packages from the all_candidates that meet the specified
        // criteria and add them to the candidate_pkg_infos.
        //
        // Since `all_candidates` includes the ten packages installed locally,
        // searching through `all_candidates` allows those locally installed
        // packages to be added to `new_pkgs_to_be_searched`, ensuring that the
        // dependencies within those packages are processed.
        if let Some(candidates) = all_candidates.get(&dependency.pkg_identity) {
            for candidate in candidates {
                if dependency.version_req.matches(&candidate.version) {
                    candidate_pkg_infos.push(candidate.clone());
                }
            }
        }

        for mut candidate_pkg_info in candidate_pkg_infos {
            let compatible_score = is_pkg_supports_compatible_with(
                &candidate_pkg_info.supports,
                support,
            );

            // A package is considered a candidate only when compatible_score >=
            // 0.
            if compatible_score >= 0 {
                // Record the package's compatible_score so that we can later
                // select the most appropriate one.
                candidate_pkg_info.compatible_score = compatible_score;

                all_candidates
                    .entry(dependency.pkg_identity.clone())
                    .or_default()
                    .insert(candidate_pkg_info.clone());

                new_pkgs_to_be_searched.push(candidate_pkg_info);
            }
        }
    }

    Ok(())
}

/// For a version of a package, only keep the most suitable one. The most
/// suitable one is the package with the highest compatible_score. If there are
/// multiple packages with the highest score, just pick one at random.
fn clean_up_all_candidates(
    all_candidates: &mut HashMap<PkgIdentity, HashSet<PkgInfo>>,
    locked_pkgs: Option<&HashMap<PkgIdentity, PkgInfo>>,
) {
    for (pkg_identity, pkg_infos) in all_candidates.iter_mut() {
        let mut version_map: HashMap<Version, &PkgInfo> = HashMap::new();
        let mut locked_pkgs_map: HashMap<Version, &PkgInfo> = HashMap::new();

        let locked_pkg =
            locked_pkgs.and_then(|locked_pkgs| locked_pkgs.get(pkg_identity));

        for pkg_info in pkg_infos.iter() {
            // Check if the candidate is a locked one.
            if let Some(locked_pkg) = locked_pkg {
                if locked_pkg.version == pkg_info.version
                    && locked_pkg.hash == pkg_info.hash
                {
                    locked_pkgs_map.insert(pkg_info.version.clone(), pkg_info);
                }
            }

            version_map
                .entry(pkg_info.version.clone())
                .and_modify(|existing_pkg_info| {
                    if pkg_info.compatible_score
                        > existing_pkg_info.compatible_score
                    {
                        *existing_pkg_info = pkg_info;
                    }
                })
                .or_insert(pkg_info);
        }

        // If there is a locked one, replace the candidate with the highest
        // score with the locked one.
        version_map.extend(locked_pkgs_map);

        *pkg_infos = version_map.into_values().cloned().collect();
    }
}

pub async fn get_all_candidates_from_deps(
    tman_config: &TmanConfig,
    support: &PkgSupport,
    mut pkgs_to_be_searched: Vec<PkgInfo>,
    mut all_candidates: HashMap<PkgIdentity, HashSet<PkgInfo>>,
    extra_dependencies: &Vec<PkgDependency>,
    locked_pkgs: Option<&HashMap<PkgIdentity, PkgInfo>>,
) -> Result<HashMap<PkgIdentity, HashSet<PkgInfo>>> {
    let mut merged_dependencies = HashSet::<FoundDependency>::new();
    let mut processed_pkgs = HashSet::<PkgInfo>::new();

    // If there is extra dependencies (ex: specified in the command line),
    // handle those dependencies, too.
    if !extra_dependencies.is_empty() {
        let mut new_pkgs_to_be_searched: Vec<PkgInfo> = vec![];

        process_dependencies_to_get_candidates(
            tman_config,
            support,
            extra_dependencies,
            &mut merged_dependencies,
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
            if processed_pkgs.contains(&pkg_to_be_search) {
                // If this package info has already been processed, do not
                // process it again.
                continue;
            }

            process_dependencies_to_get_candidates(
                tman_config,
                support,
                &pkg_to_be_search.dependencies,
                &mut merged_dependencies,
                &mut all_candidates,
                &mut new_pkgs_to_be_searched,
            )
            .await?;

            // Remember that this package has already been processed.
            processed_pkgs.insert(pkg_to_be_search);
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
    all_candidates: &HashMap<PkgIdentity, HashSet<PkgInfo>>,
) -> Result<PkgInfo> {
    let pkg_type_enum = pkg_type.parse::<PkgType>()?;
    let pkg_identity = PkgIdentity {
        pkg_type: pkg_type_enum,
        name: pkg_name.to_string(),
    };
    let version_parsed = Version::parse(version)?;
    let pkg_info = all_candidates
        .get(&pkg_identity)
        .and_then(|set| set.iter().find(|pkg| pkg.version == version_parsed))
        .ok_or_else(|| {
            anyhow!(
                "PkgInfo not found for [{}]{}@{}",
                pkg_type,
                pkg_name,
                version
            )
        })?;
    Ok(pkg_info.clone())
}
