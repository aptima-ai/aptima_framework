//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
use std::collections::{HashMap, HashSet};

use anyhow::Result;
use regex::Regex;

use ten_rust::pkg_info::{pkg_identity::PkgIdentity, PkgInfo};

use crate::dep_and_candidate::get_pkg_info_from_candidates;

pub fn extract_introducer_relations_from_raw_solver_results(
    results: &[String],
    all_candidates: &HashMap<PkgIdentity, HashSet<PkgInfo>>,
) -> Result<HashMap<PkgInfo, Option<PkgInfo>>> {
    let re = Regex::new(
      r#"introducer\("([^"]+)","([^"]+)","([^"]+)","([^"]+)","([^"]*)","([^"]*)"\)"#,
  )
  .unwrap();

    let mut introducer_relations: HashMap<PkgInfo, Option<PkgInfo>> =
        HashMap::new();

    for result in results.iter() {
        if let Some(caps) = re.captures(result) {
            let pkg_type_str = caps.get(1).unwrap().as_str();
            let name = caps.get(2).unwrap().as_str();
            let version_str = caps.get(3).unwrap().as_str();

            let introducer_type_str = caps.get(4).unwrap().as_str();
            let introducer_name = caps.get(5).unwrap().as_str();
            let introducer_version_str = caps.get(6).unwrap().as_str();

            let pkg_info = get_pkg_info_from_candidates(
                pkg_type_str,
                name,
                version_str,
                all_candidates,
            )?;

            let introducer = if introducer_type_str == "root" {
                None
            } else {
                let introducer_info = get_pkg_info_from_candidates(
                    introducer_type_str,
                    introducer_name,
                    introducer_version_str,
                    all_candidates,
                )?;

                Some(introducer_info.clone())
            };

            introducer_relations.insert(pkg_info.clone(), introducer);
        }
    }

    Ok(introducer_relations)
}

pub fn get_dependency_chain(
    pkg_info: &PkgInfo,
    introducer_relations: &HashMap<PkgInfo, Option<PkgInfo>>,
) -> Vec<PkgInfo> {
    let mut chain = Vec::new();
    let mut current_pkg = pkg_info.clone();
    let mut visited = HashSet::new();

    loop {
        if !visited.insert(current_pkg.clone()) {
            // Detected a cycle, break to prevent infinite loop.
            break;
        }

        chain.push(current_pkg.clone());

        match introducer_relations.get(&current_pkg) {
            Some(Some(introducer_pkg)) => {
                current_pkg = introducer_pkg.clone();
            }
            Some(None) => {
                // Reached the root.
                break;
            }
            None => {
                panic!("Introducer not found, possibly an error.");
            }
        }
    }

    // Reverse the chain to start from the root.
    chain.reverse();
    chain
}
