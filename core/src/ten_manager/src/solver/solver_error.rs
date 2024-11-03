//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
use std::collections::{HashMap, HashSet};

use anyhow::{anyhow, Result};
use console::Emoji;
use regex::Regex;

use ten_rust::pkg_info::{pkg_identity::PkgIdentity, PkgInfo};

use crate::{
    dep_and_candidate::get_pkg_info_from_candidates,
    solver::introducer::get_dependency_chain,
};

#[derive(Debug)]
pub struct ConflictInfo {
    pub pkg_type: String,
    pub pkg_name: String,
    pub version1: String,
    pub version2: String,
    pub error_message: String,
}

pub fn parse_error_statement(clingo_output: &[String]) -> Result<ConflictInfo> {
    // Regular expression to match the error statement.
    let error_re = Regex::new(
        r#"error\(\s*\d+,\s*"([^"]+)",\s*"([^"]+)",\s*"([^"]+)",\s*"([^"]+)",\s*"([^"]+)",\s*"([^"]+)",\s*"([^"]+)",\s*"([^"]+)",\s*"([^"]+)",\s*"([^"]+)",\s*"([^"]+)"\)"#,
    )?;

    for line in clingo_output {
        if let Some(caps) = error_re.captures(line) {
            let error_template = caps.get(1).unwrap().as_str();
            let pkg_type = caps.get(2).unwrap().as_str().to_string();
            let pkg_name = caps.get(3).unwrap().as_str().to_string();
            let version1 = caps.get(4).unwrap().as_str().to_string();
            let version2 = caps.get(5).unwrap().as_str().to_string();
            let introducer1_type = caps.get(6).unwrap().as_str().to_string();
            let introducer1_name = caps.get(7).unwrap().as_str().to_string();
            let introducer1_version = caps.get(8).unwrap().as_str().to_string();
            let introducer2_type = caps.get(9).unwrap().as_str().to_string();
            let introducer2_name = caps.get(10).unwrap().as_str().to_string();
            let introducer2_version =
                caps.get(11).unwrap().as_str().to_string();

            // Format the error message with the extracted arguments.
            let error_message = error_template
                .replace("{0}", &pkg_type)
                .replace("{1}", &pkg_name)
                .replace("{2}", &version1)
                .replace("{3}", &version2)
                .replace("{4}", &introducer1_type)
                .replace("{5}", &introducer1_name)
                .replace("{6}", &introducer1_version)
                .replace("{7}", &introducer2_type)
                .replace("{8}", &introducer2_name)
                .replace("{9}", &introducer2_version);

            return Ok(ConflictInfo {
                pkg_type,
                pkg_name,
                version1,
                version2,
                error_message,
            });
        }
    }

    Err(anyhow!("No error message found in the Clingo output."))
}

fn print_dependency_chain(
    chain: &[PkgInfo],
    pkg_type: &str,
    pkg_name: &str,
    version: &str,
) {
    println!(
        "Dependency chain leading to [{}]{}@{}:",
        pkg_type, pkg_name, version
    );
    for (i, pkg) in chain.iter().enumerate() {
        println!(
            "{:indent$}└─ [{}]{}@{}",
            "",
            pkg.pkg_identity.pkg_type,
            pkg.pkg_identity.name,
            pkg.version,
            indent = i * 3
        );
    }
    println!();
}

pub fn print_conflict_info(
    conflict_info: &ConflictInfo,
    introducer_relations: &HashMap<PkgInfo, Option<PkgInfo>>,
    all_candidates: &HashMap<PkgIdentity, HashSet<PkgInfo>>,
) -> Result<()> {
    println!(
        "{}  Error: {}",
        Emoji("❌", ":-("),
        conflict_info.error_message
    );
    println!();

    // Get PkgInfo for both versions
    let pkg_info1 = get_pkg_info_from_candidates(
        &conflict_info.pkg_type,
        &conflict_info.pkg_name,
        &conflict_info.version1,
        all_candidates,
    )?;
    let pkg_info2 = get_pkg_info_from_candidates(
        &conflict_info.pkg_type,
        &conflict_info.pkg_name,
        &conflict_info.version2,
        all_candidates,
    )?;

    // Get dependency chains
    let chain1 = get_dependency_chain(&pkg_info1, introducer_relations);
    let chain2 = get_dependency_chain(&pkg_info2, introducer_relations);

    print_dependency_chain(
        &chain1,
        &conflict_info.pkg_type,
        &conflict_info.pkg_name,
        &conflict_info.version1,
    );

    print_dependency_chain(
        &chain2,
        &conflict_info.pkg_type,
        &conflict_info.pkg_name,
        &conflict_info.version2,
    );

    Ok(())
}
