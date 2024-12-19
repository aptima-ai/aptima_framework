//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
use std::{
    collections::{HashMap, HashSet},
    path::Path,
};

use anyhow::Result;
use console::Emoji;
use indicatif::{ProgressBar, ProgressStyle};
use regex::Regex;
use semver::Version;

use ten_rust::pkg_info::{
    pkg_type::PkgType, pkg_type_and_name::PkgTypeAndName, PkgInfo,
};

use crate::{
    config::TmanConfig,
    constants::{
        EXTENSION_DIR, EXTENSION_GROUP_DIR, PROTOCOL_DIR, SYSTEM_DIR,
        TEN_PACKAGES_DIR,
    },
    install::{install_pkg_info, PkgIdentityMapping},
};

pub fn extract_solver_results_from_raw_solver_results(
    results: &[String],
    all_candidates: &HashMap<PkgTypeAndName, HashSet<PkgInfo>>,
) -> Result<Vec<PkgInfo>> {
    let re =
        Regex::new(r#"selected_pkg_version\("([^"]+)","([^"]+)","([^"]+)"\)"#)
            .unwrap();

    let mut results_info: Vec<PkgInfo> = vec![];

    'outer: for result in results.iter() {
        if let Some(caps) = re.captures(result) {
            let pkg_type_str = caps.get(1).map_or("", |m| m.as_str());
            let name = caps.get(2).map_or("", |m| m.as_str());
            let semver_str = caps.get(3).map_or("", |m| m.as_str());

            let pkg_type = pkg_type_str.parse::<PkgType>()?;
            let semver = semver_str.parse::<Version>()?;

            for candidate in all_candidates
                .get(&PkgTypeAndName {
                    pkg_type,
                    name: name.to_string(),
                })
                .unwrap()
            {
                if candidate.pkg_type != pkg_type || candidate.name != name {
                    panic!("Should not happen.");
                }

                if candidate.version == semver {
                    results_info.push(candidate.clone());
                    continue 'outer;
                }
            }
        }
    }

    Ok(results_info)
}

pub fn filter_solver_results_by_type_and_name<'a>(
    solver_results: &'a [PkgInfo],
    pkg_type: Option<&PkgType>,
    name: Option<&String>,
    filter_in: bool,
) -> Result<Vec<&'a PkgInfo>> {
    let mut filtered_results: Vec<&PkgInfo> = vec![];

    for result in solver_results.iter() {
        let matches_type = pkg_type.map_or(true, |pt| result.pkg_type == *pt);
        let matches_name = name.map_or(true, |n| result.name == *n);

        let matches = matches_type && matches_name;

        if filter_in {
            if matches {
                filtered_results.push(result);
            }
        } else if !matches {
            filtered_results.push(result);
        }
    }

    Ok(filtered_results)
}

pub async fn install_solver_results_in_app_folder(
    tman_config: &TmanConfig,
    solver_results: &Vec<&PkgInfo>,
    pkg_identity_mappings: &Vec<PkgIdentityMapping>,
    template_ctx: Option<&serde_json::Value>,
    app_dir: &Path,
) -> Result<()> {
    println!("{}  Installing packages...", Emoji("📥", "+"));

    let bar = ProgressBar::new(solver_results.len().try_into()?);
    bar.set_style(
        ProgressStyle::default_bar()
            .template("{spinner:.green} [{elapsed_precise}] [{bar:40.cyan/blue}] {pos:>7}/{len:7} {msg}")?
            .progress_chars("#>-"),
    );

    for solver_result in solver_results {
        bar.inc(1);
        bar.set_message(format!(
            "{}/{}",
            solver_result.pkg_type, solver_result.name
        ));

        let base_dir = match solver_result.pkg_type {
            PkgType::Extension => {
                app_dir.join(TEN_PACKAGES_DIR).join(EXTENSION_DIR)
            }
            PkgType::ExtensionGroup => {
                app_dir.join(TEN_PACKAGES_DIR).join(EXTENSION_GROUP_DIR)
            }
            PkgType::Protocol => {
                app_dir.join(TEN_PACKAGES_DIR).join(PROTOCOL_DIR)
            }
            PkgType::System => app_dir.join(TEN_PACKAGES_DIR).join(SYSTEM_DIR),
            PkgType::App => app_dir.to_path_buf(),
        };

        install_pkg_info(
            tman_config,
            solver_result,
            pkg_identity_mappings,
            template_ctx,
            &base_dir,
        )
        .await?;
    }

    bar.finish_with_message("Done");

    Ok(())
}

pub async fn install_solver_results_in_standalone_mode(
    tman_config: &TmanConfig,
    solver_results: &Vec<&PkgInfo>,
    pkg_identity_mappings: &Vec<PkgIdentityMapping>,
    template_ctx: Option<&serde_json::Value>,
    dir: &Path,
) -> Result<()> {
    for solver_result in solver_results {
        install_pkg_info(
            tman_config,
            solver_result,
            pkg_identity_mappings,
            template_ctx,
            dir,
        )
        .await?;
    }

    Ok(())
}
