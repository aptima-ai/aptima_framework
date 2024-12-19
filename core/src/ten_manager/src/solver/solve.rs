//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
use std::collections::{HashMap, HashSet};

use anyhow::Result;
use clingo::{
    control, Configuration, ConfigurationType, Id, Model, Part, ShowType,
    SolveMode, Statistics, StatisticsType,
};

use ten_rust::pkg_info::{
    dependencies::DependencyRelationship, pkg_type::PkgType,
    pkg_type_and_name::PkgTypeAndName, PkgInfo,
};

use crate::{
    config::TmanConfig,
    error::TmanError,
    log::{tman_verbose_print, tman_verbose_println},
};

fn get_model(
    tman_config: &TmanConfig,
    model: &Model,
    is_usable: &mut bool,
) -> Option<Vec<String>> {
    // Retrieve the symbols in the model.
    let atoms = model
        .symbols(ShowType::SHOWN)
        .expect("Failed to retrieve symbols in the model.");

    tman_verbose_println!(tman_config, "Model:");

    let mut result = Vec::new();
    *is_usable = true;

    for symbol in atoms {
        tman_verbose_println!(tman_config, " {}", symbol);

        result.push(symbol.to_string());
        if symbol.to_string().starts_with("error(") {
            *is_usable = false;
        }
    }
    tman_verbose_println!(tman_config, "");

    Some(result)
}

#[allow(dead_code)]
fn print_prefix(tman_config: &TmanConfig, depth: u8) {
    tman_verbose_println!(tman_config, "");
    for _ in 0..depth {
        tman_verbose_print!(tman_config, "  ");
    }
}

// Recursively print the configuration object.
#[allow(dead_code)]
fn print_configuration(
    tman_config: &TmanConfig,
    conf: &Configuration,
    key: Id,
    depth: u8,
) {
    let configuration_type = conf.configuration_type(key).unwrap();
    if configuration_type.contains(ConfigurationType::VALUE) {
        let value = conf
            .value_get(key)
            .expect("Failed to retrieve statistics value.");

        tman_verbose_print!(tman_config, "{}", value);
    } else if configuration_type.contains(ConfigurationType::ARRAY) {
        let size = conf
            .array_size(key)
            .expect("Failed to retrieve statistics array size.");
        for i in 0..size {
            let subkey = conf
                .array_at(key, i)
                .expect("Failed to retrieve statistics array.");
            print_prefix(tman_config, depth);
            tman_verbose_print!(tman_config, "{}: ", i);

            print_configuration(tman_config, conf, subkey, depth + 1);
        }
    } else if configuration_type.contains(ConfigurationType::MAP) {
        let size = conf.map_size(key).unwrap();
        for i in 0..size {
            let name = conf.map_subkey_name(key, i).unwrap();
            let subkey = conf.map_at(key, name).unwrap();
            print_prefix(tman_config, depth);
            tman_verbose_print!(tman_config, "{}: ", name);

            print_configuration(tman_config, conf, subkey, depth + 1);
        }
    } else {
        tman_verbose_println!(tman_config, "Unknown ConfigurationType");
        unreachable!()
    }
}

// recursively print the statistics object
#[allow(dead_code)]
fn print_statistics(
    tman_config: &TmanConfig,
    stats: &Statistics,
    key: u64,
    depth: u8,
) {
    // Get the type of an entry and switch over its various values.
    let statistics_type = stats.statistics_type(key).unwrap();
    match statistics_type {
        StatisticsType::Value => {
            let value = stats
                .value_get(key)
                .expect("Failed to retrieve statistics value.");

            tman_verbose_print!(tman_config, " {}", value);
        }

        StatisticsType::Array => {
            let size = stats
                .array_size(key)
                .expect("Failed to retrieve statistics array size.");
            for i in 0..size {
                let subkey = stats
                    .array_at(key, i)
                    .expect("Failed to retrieve statistics array.");
                print_prefix(tman_config, depth);
                tman_verbose_print!(tman_config, "{} zu:", i);

                print_statistics(tman_config, stats, subkey, depth + 1);
            }
        }

        StatisticsType::Map => {
            let size = stats.map_size(key).unwrap();
            for i in 0..size {
                let name = stats.map_subkey_name(key, i).unwrap();
                let subkey = stats.map_at(key, name).unwrap();
                print_prefix(tman_config, depth);
                tman_verbose_print!(tman_config, "{}:", name);

                print_statistics(tman_config, stats, subkey, depth + 1);
            }
        }

        StatisticsType::Empty => {
            tman_verbose_println!(tman_config, "StatisticsType::Empty");
        }
    }
}

type UsableModel = Option<Vec<String>>;
type NonUsableModels = Vec<Vec<String>>;
type SolveOutcome = (UsableModel, NonUsableModels);
type SolveResult = Result<SolveOutcome>;

#[allow(unused_assignments)]
fn solve(tman_config: &TmanConfig, input: &str) -> SolveResult {
    // Create a control object.
    // i.e., clingo_control_new
    let mut ctl = control({
        let mut args = vec![];

        if tman_config.verbose {
            args.push("--verbose".to_string());
        }

        args
    })
    .expect("Failed creating Control.");

    {
        // Get the configuration object and its root key.
        let conf = ctl.configuration_mut().unwrap();
        let root_key = conf.root().unwrap();

        // Configure to enumerate all models.
        let solve_models_key = conf.map_at(root_key, "solve.models").unwrap();
        conf.value_set(solve_models_key, "0")
            .expect("Failed to set solve.models to 0.");

        // Configure to enumerate all optimal models.
        let solve_opt_mode_key =
            conf.map_at(root_key, "solve.opt_mode").unwrap();
        conf.value_set(solve_opt_mode_key, "optN")
            .expect("Failed to set solve.opt_mode to optN.");

        // Enable full statistics.
        let stats_key = conf.map_at(root_key, "stats").unwrap();
        conf.value_set(stats_key, "2")
            .expect("Failed to set stats_key to 2.");

        // Configure the first solver to use the berkmin heuristic.
        let mut solver_key = conf.map_at(root_key, "solver").unwrap();
        solver_key = conf.array_at(solver_key, 0).unwrap();
        let heuristic_key = conf.map_at(solver_key, "heuristic").unwrap();
        conf.value_set(heuristic_key, "berkmin")
            .expect("Failed to set heuristic to berkmin.");

        // print_configuration(tman_config, conf, root_key, 0);
        // tman_verbose_println!(tman_config, "");
    }

    let main_program = include_str!("main.lp");
    let display_program = include_str!("display.lp");

    // Add a logic program to the base part.
    // i.e., clingo_control_add
    ctl.add("main", &[], main_program)
        .expect("Failed to add main.lp");
    ctl.add("display", &[], display_program)
        .expect("Failed to add display.lp");
    ctl.add("base", &[], input).expect("Failed to add input.lp");

    // Ground the parts.
    // i.e., clingo_control_ground
    let main_part = Part::new("main", vec![]).unwrap();
    let display_part = Part::new("display", vec![]).unwrap();
    let base_part = Part::new("base", vec![]).unwrap();

    let parts = vec![main_part, display_part, base_part];
    ctl.ground(&parts)
        .expect("Failed to ground a logic program.");

    // Solving. Get a solve handle.
    // i.e., clingo_control_solve
    let mut handle = ctl
        .solve(SolveMode::YIELD, &[])
        .expect("Failed retrieving solve handle.");

    let mut usable_model = None;
    let mut non_usable_models = Vec::new();

    // Loop over all models.
    loop {
        // i.e., clingo_solve_handle_resume
        handle.resume().expect("Failed resume on solve handle.");

        // i.e., clingo_solve_handle_model
        match handle.model() {
            // Get the model.
            Ok(Some(model)) => {
                let mut is_usable = false;
                if let Some(m) = get_model(tman_config, model, &mut is_usable) {
                    if is_usable {
                        usable_model = Some(m);

                        // Clingo will only output increasingly optimized
                        // models, so we need to take the last model without an
                        // error statement from all outputted models, not just
                        // the first good one. Therefore, we shouldn’t break
                        // here.
                    } else {
                        non_usable_models.push(m); // Collect error models.
                    }
                }
            }
            // Stop if there are no more models.
            Ok(None) => {
                tman_verbose_println!(tman_config, "No more models");
                break;
            }
            Err(e) => panic!("Error: {}", e),
        }
    }

    // Close the solve handle.
    // i.e., clingo_solve_handle_get
    let result = handle
        .get()
        .expect("Failed to get result from solve handle.");

    tman_verbose_println!(tman_config, "{:?}", result);

    // Free the solve handle.
    // i.e., clingo_solve_handle_close
    ctl = handle.close().expect("Failed to close solve handle.");

    // Get the statistics object, get the root key, then print the statistics
    // recursively.
    // let stats = ctl.statistics().unwrap();
    // let stats_key = stats.root().unwrap();
    // print_statistics(tman_config, stats, stats_key, 0);

    Ok((usable_model, non_usable_models))
}

fn create_input_str_for_dependency_relationship(
    input_str: &mut String,
    dependency_relationships: &Vec<DependencyRelationship>,
    all_candidates: &HashMap<PkgTypeAndName, HashSet<PkgInfo>>,
) -> Result<()> {
    for dependency_relationship in dependency_relationships {
        let candidates =
            all_candidates.get(&(&dependency_relationship.dependency).into());

        if let Some(candidates) = candidates {
            for candidate in candidates.iter() {
                if dependency_relationship
                    .dependency
                    .version_req
                    .matches(&candidate.version)
                {
                    input_str.push_str(&format!(
        "depends_on_declared(\"{}\", \"{}\", \"{}\", \"{}\", \"{}\", \"{}\").\n",
        dependency_relationship.pkg_type,
        dependency_relationship.name,
        dependency_relationship.version,
        candidate.pkg_type,
        candidate.name,
        candidate.version,
                  ));
                }
            }
        } else {
            return Err(TmanError::Custom(
                format!(
                    "Failed to find candidates for {}:{}@{}",
                    dependency_relationship.dependency.pkg_type,
                    dependency_relationship.dependency.name,
                    dependency_relationship.version,
                )
                .to_string(),
            )
            .into());
        }
    }

    Ok(())
}

fn create_input_str_for_pkg_info_dependencies(
    input_str: &mut String,
    pkg_info: &PkgInfo,
    dumped_pkgs_info: &mut HashSet<PkgInfo>,
    all_candidates: &HashMap<PkgTypeAndName, HashSet<PkgInfo>>,
) -> Result<()> {
    // If this package has already been dumped, skip it.
    if dumped_pkgs_info.contains(pkg_info) {
        return Ok(());
    }
    dumped_pkgs_info.insert(pkg_info.clone());

    for dependency in &pkg_info.dependencies {
        let candidates = all_candidates.get(&(dependency).into());

        if let Some(candidates) = candidates {
            let mut found_matched = false;

            for candidate in candidates {
                if dependency.version_req.matches(&candidate.version) {
                    input_str.push_str(&format!(
        "depends_on_declared(\"{}\", \"{}\", \"{}\", \"{}\", \"{}\", \"{}\").\n",
        pkg_info.pkg_type,
        pkg_info.name,
        pkg_info.version,
        candidate.pkg_type,
        candidate.name,
        candidate.version,
                  ));

                    create_input_str_for_pkg_info_dependencies(
                        input_str,
                        candidate,
                        dumped_pkgs_info,
                        all_candidates,
                    )?;

                    found_matched = true;
                }
            }

            if !found_matched {
                return Err(TmanError::Custom(
                    format!(
                        "Failed to find candidates for [{}]{}({})",
                        dependency.pkg_type,
                        dependency.name,
                        dependency.version_req
                    )
                    .to_string(),
                )
                .into());
            }
        } else {
            return Err(TmanError::Custom(
                format!(
                    "Failed to find candidates for {}:{}@{}",
                    dependency.pkg_type,
                    dependency.name,
                    dependency.version_req
                )
                .to_string(),
            )
            .into());
        }
    }

    Ok(())
}

fn create_input_str_for_pkg_info_without_dependencies(
    input_str: &mut String,
    pkg_info: &PkgInfo,
    weight: &usize,
) -> Result<()> {
    input_str.push_str(&format!(
        "version_declared(\"{}\", \"{}\", \"{}\", {}).\n",
        pkg_info.pkg_type, pkg_info.name, pkg_info.version, weight
    ));

    Ok(())
}

fn create_input_str_for_all_possible_pkgs_info(
    input_str: &mut String,
    all_candidates: &HashMap<PkgTypeAndName, HashSet<PkgInfo>>,
    locked_pkgs: Option<&HashMap<PkgTypeAndName, PkgInfo>>,
) -> Result<()> {
    for candidates in all_candidates {
        let mut candidates_vec: Vec<_> = candidates.1.iter().collect();

        // The sorting below places the larger versions at the front, thus
        // having smaller indexes. This is correct because, in the Clingo
        // solver, our optimization strategy is to minimize the overall weight,
        // and we prefer larger version numbers. Therefore, larger version
        // numbers have smaller weights, and the index here is equivalent to the
        // concept of weight in the Clingo solver.
        candidates_vec.sort_by(|a, b| b.cmp(a));

        // Check if the locked package exists in the candidates. If it does,
        // move it to the front of the candidates_vec so that it has a smaller
        // weight.
        let locked_pkg =
            locked_pkgs.and_then(|locked_pkgs| locked_pkgs.get(candidates.0));

        if let Some(locked_pkg) = locked_pkg {
            let idx = candidates_vec
                .iter()
                .position(|pkg_info| pkg_info.version == locked_pkg.version);

            if let Some(idx) = idx {
                candidates_vec.remove(idx);
                candidates_vec.insert(0, locked_pkg);
            }
        }

        for (idx, candidate) in candidates_vec.into_iter().enumerate() {
            create_input_str_for_pkg_info_without_dependencies(
                input_str, candidate, &idx,
            )?;
        }
    }

    Ok(())
}

fn create_input_str(
    tman_config: &TmanConfig,
    pkg_name: &String,
    pkg_type: &PkgType,
    extra_dependency_relationships: &Vec<DependencyRelationship>,
    all_candidates: &HashMap<PkgTypeAndName, HashSet<PkgInfo>>,
    locked_pkgs: Option<&HashMap<PkgTypeAndName, PkgInfo>>,
) -> Result<String> {
    let mut input_str = String::new();

    input_str.push_str(&format!(
        "root_declared(\"{}\", \"{}\").\n",
        pkg_type, pkg_name,
    ));

    create_input_str_for_all_possible_pkgs_info(
        &mut input_str,
        all_candidates,
        locked_pkgs,
    )?;

    create_input_str_for_dependency_relationship(
        &mut input_str,
        extra_dependency_relationships,
        all_candidates,
    )?;

    let mut dumped_pkgs_info = HashSet::new();

    for candidates in all_candidates {
        for candidate in candidates.1 {
            create_input_str_for_pkg_info_dependencies(
                &mut input_str,
                candidate,
                &mut dumped_pkgs_info,
                all_candidates,
            )?;
        }
    }

    if tman_config.verbose {
        tman_verbose_println!(tman_config, "Input: \n{}", input_str);

        // dump_string_to_file_if_debug(
        //     tman_config,
        //     &input_str,
        //     Path::new("input.lp"),
        // )?;
    }

    Ok(input_str)
}

pub fn solve_all(
    tman_config: &TmanConfig,
    pkg_name: &String,
    pkg_type: &PkgType,
    extra_dependency_relationships: &Vec<DependencyRelationship>,
    all_candidates: &HashMap<PkgTypeAndName, HashSet<PkgInfo>>,
    locked_pkgs: Option<&HashMap<PkgTypeAndName, PkgInfo>>,
) -> SolveResult {
    let input_str = create_input_str(
        tman_config,
        pkg_name,
        pkg_type,
        extra_dependency_relationships,
        all_candidates,
        locked_pkgs,
    )?;
    solve(tman_config, &input_str)
}
