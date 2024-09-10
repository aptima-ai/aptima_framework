//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
use std::collections::{HashMap, HashSet};

use anyhow::Result;

use crate::{config::TmanConfig, error::TmanError, log::tman_verbose_println};
use ten_rust::pkg_info::{
    dependencies::DependencyRelationship, pkg_identity::PkgIdentity,
    pkg_type::PkgType, PkgInfo,
};

fn get_model(
    tman_config: &TmanConfig,
    model: &clingo::Model,
) -> Option<Vec<String>> {
    // Retrieve the symbols in the model.
    let atoms = model
        .symbols(clingo::ShowType::SHOWN)
        .expect("Failed to retrieve symbols in the model.");

    tman_verbose_println!(tman_config, "Model:");

    let mut result = Vec::new();
    let mut can_be_used = true;

    for symbol in atoms {
        tman_verbose_println!(tman_config, " {}", symbol);

        result.push(symbol.to_string());
        if symbol.to_string().starts_with("error(") {
            can_be_used = false;
        }
    }
    tman_verbose_println!(tman_config, "");

    if !can_be_used {
        return None;
    }
    Some(result)
}

fn solve(tman_config: &TmanConfig, input: &str) -> Result<Vec<String>> {
    // Create a control object.
    // i.e., clingo_control_new
    let mut ctl =
        clingo::control([].to_vec()).expect("Failed creating Control.");

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
    let main_part = clingo::Part::new("main", vec![]).unwrap();
    let display_part = clingo::Part::new("display", vec![]).unwrap();
    let base_part = clingo::Part::new("base", vec![]).unwrap();

    let parts = vec![main_part, display_part, base_part];
    ctl.ground(&parts)
        .expect("Failed to ground a logic program.");

    let conf = ctl.configuration_mut()?;

    // Navigate to the solve.models setting and modify it.
    let root_key = conf.root()?;
    let solve_key = conf.map_at(root_key, "solve")?;
    let models_key = conf.map_at(solve_key, "models")?;
    // Setting it to "0" means finding all models.
    conf.value_set(models_key, "0")?;

    // Solving. Get a solve handle.
    // i.e., clingo_control_solve
    let mut handle = ctl
        .solve(clingo::SolveMode::YIELD, &[])
        .expect("Failed retrieving solve handle.");

    let mut result_model = Vec::new();

    // Loop over all models
    loop {
        // i.e., clingo_solve_handle_resume
        handle.resume().expect("Failed resume on solve handle.");

        // i.e., clingo_solve_handle_model
        match handle.model() {
            // print the model
            Ok(Some(model)) => {
                if let Some(m) = get_model(tman_config, model) {
                    result_model = m;
                }
            }
            // stop if there are no more models
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
    handle.close().expect("Failed to close solve handle.");

    Ok(result_model)
}

fn create_input_str_for_dependency_relationship(
    input_str: &mut String,
    dependency_relationships: &Vec<DependencyRelationship>,
    all_candidates: &HashMap<PkgIdentity, HashSet<PkgInfo>>,
) -> Result<()> {
    for dependency_relationship in dependency_relationships {
        let candidates = all_candidates
            .get(&dependency_relationship.dependency.pkg_identity);

        if let Some(candidates) = candidates {
            for candidate in candidates.iter() {
                if dependency_relationship
                    .dependency
                    .version_req
                    .matches(&candidate.version)
                {
                    input_str.push_str(&format!(
        "depends_on_declared(\"{}\", \"{}\", \"{}\", \"{}\", \"{}\", \"{}\").\n",
        dependency_relationship.pkg_identity.pkg_type,
        dependency_relationship.pkg_identity.name,
        dependency_relationship.version,
        candidate.pkg_identity.pkg_type,
        candidate.pkg_identity.name,
        candidate.version,
                  ));
                }
            }
        } else {
            return Err(TmanError::Custom(
                format!(
                    "Failed to find candidates for {}:{}@{}",
                    dependency_relationship.dependency.pkg_identity.pkg_type,
                    dependency_relationship.dependency.pkg_identity.name,
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
    all_candidates: &HashMap<PkgIdentity, HashSet<PkgInfo>>,
) -> Result<()> {
    // If this package has already been dumped, skip it.
    if dumped_pkgs_info.contains(pkg_info) {
        return Ok(());
    }
    dumped_pkgs_info.insert(pkg_info.clone());

    for dependency in &pkg_info.dependencies {
        let candidates = all_candidates.get(&dependency.pkg_identity);

        if let Some(candidates) = candidates {
            let mut found_matched = false;

            for candidate in candidates {
                if dependency.version_req.matches(&candidate.version) {
                    input_str.push_str(&format!(
        "depends_on_declared(\"{}\", \"{}\", \"{}\", \"{}\", \"{}\", \"{}\").\n",
        pkg_info.pkg_identity.pkg_type,
        pkg_info.pkg_identity.name,
        pkg_info.version,
        candidate.pkg_identity.pkg_type,
        candidate.pkg_identity.name,
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
                        "Failed to find candidates for {}::{}({})",
                        dependency.pkg_identity.pkg_type,
                        dependency.pkg_identity.name,
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
                    dependency.pkg_identity.pkg_type,
                    dependency.pkg_identity.name,
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
        pkg_info.pkg_identity.pkg_type,
        pkg_info.pkg_identity.name,
        pkg_info.version,
        weight
    ));

    Ok(())
}

fn create_input_str_for_all_possible_pkgs_info(
    input_str: &mut String,
    all_candidates: &HashMap<PkgIdentity, HashSet<PkgInfo>>,
    locked_pkgs: Option<&HashMap<PkgIdentity, PkgInfo>>,
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
    all_candidates: &HashMap<PkgIdentity, HashSet<PkgInfo>>,
    locked_pkgs: Option<&HashMap<PkgIdentity, PkgInfo>>,
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
    all_candidates: &HashMap<PkgIdentity, HashSet<PkgInfo>>,
    locked_pkgs: Option<&HashMap<PkgIdentity, PkgInfo>>,
) -> Result<Vec<String>> {
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
