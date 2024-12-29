//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
use std::{collections::HashMap, fs, path, str::FromStr};

use anyhow::{Context, Result};
use clap::{Arg, ArgMatches, Command};
use console::Emoji;

use ten_rust::pkg_info::{
    get_all_existed_pkgs_info_of_app, graph::Graph, localhost,
    property::parse_property_in_folder, PkgInfo,
};

use crate::config::TmanConfig;

#[derive(Debug)]
pub struct CheckGraphCommand {
    pub app_dir: Vec<String>,
    pub predefined_graph_name: Option<String>,
    pub graph_json_str: Option<String>,
}

pub fn create_sub_cmd(_args_cfg: &crate::cmd_line::ArgsCfg) -> Command {
    Command::new("graph")
        .about(
            "Check whether the graph content of the predefined graph or start_graph command is correct",
        )
        .arg(
            Arg::new("APP_DIR")
                .long("app-dir")
                .help(
                    "The absolute path of the app declared in the graph. By \
                    default, the predefined graph will be read from the first \
                    one in the list.",
                )
                .required(true).num_args(1..),
        )
        .arg(
            Arg::new("PREDEFINED_GRAPH_NAME")
                .long("predefined-graph-name")
                .help(
                    "Specify the predefined graph name only to be checked, \
                    otherwise, all predefined graphs will be checked.",
                )
                .required(false)
                .conflicts_with("GRAPH_JSON_STR"),
        )
        .arg(
            Arg::new("GRAPH_JSON_STR")
                .long("graph-json-str")
                .help(
                    "Specify the json string of a 'start_graph' cmd to be \
                    checked. If not specified, the predefined graph in the \
                    first app will be checked.",
                )
                .required(false)
                .conflicts_with("PREDEFINED_GRAPH_NAME"),
        )
}

pub fn parse_sub_cmd(sub_cmd_args: &ArgMatches) -> CheckGraphCommand {
    let cmd = CheckGraphCommand {
        app_dir: sub_cmd_args
            .get_many::<String>("APP_DIR")
            .unwrap_or_default()
            .map(|s| s.to_string())
            .collect(),
        predefined_graph_name: sub_cmd_args
            .get_one::<String>("PREDEFINED_GRAPH_NAME")
            .cloned(),
        graph_json_str: sub_cmd_args
            .get_one::<String>("GRAPH_JSON_STR")
            .cloned(),
    };

    cmd
}

fn validate_cmd_args(command: &CheckGraphCommand) -> Result<()> {
    for app in &command.app_dir {
        let stat = fs::metadata(app).with_context(|| {
            format!("Failed to get metadata of app path [{}].", app)
        })?;
        if !stat.is_dir() {
            return Err(anyhow::anyhow!(
                "App path [{}] is not a directory.",
                app
            ));
        }
    }

    Ok(())
}

fn get_existed_pkgs_of_all_apps(
    command: &CheckGraphCommand,
) -> Result<HashMap<String, Vec<PkgInfo>>> {
    let mut pkgs_info: HashMap<String, Vec<PkgInfo>> = HashMap::new();

    let single_app = command.app_dir.len() == 1;

    for app in &command.app_dir {
        let app_path = path::Path::new(app);
        let app_existed_pkgs = get_all_existed_pkgs_info_of_app(app_path)?;

        let app_property = parse_property_in_folder(app_path)?;
        let app_uri = if let Some(property) = app_property {
            property.get_app_uri()
        } else {
            localhost()
        };

        if !single_app && app_uri.as_str() == localhost() {
            return Err(anyhow::anyhow!(
                "The app uri should be some string other than 'localhost' when
                using in multi-apps graph."
            ));
        }

        let present_pkg = pkgs_info.insert(app_uri.clone(), app_existed_pkgs);
        if present_pkg.is_some() {
            return Err(anyhow::anyhow!(
                "All apps should have a unique uri, but uri [{}] is duplicated.",
                app_uri
            ));
        }
    }

    Ok(pkgs_info)
}

fn get_graphs_to_be_checked(command: &CheckGraphCommand) -> Result<Vec<Graph>> {
    let mut graphs_to_be_checked: Vec<Graph> = Vec::new();

    if let Some(graph_str) = &command.graph_json_str {
        let graph: Graph = Graph::from_str(graph_str).map_err(|e| {
            anyhow::anyhow!("Failed to parse graph string, {}", e)
        })?;
        graphs_to_be_checked.push(graph);
    } else {
        let first_app_path = path::Path::new(&command.app_dir[0]);
        let first_app_property = parse_property_in_folder(first_app_path)?;
        if first_app_property.is_none() {
            return Err(anyhow::anyhow!("The property.json is not found in the first app, which is required to retrieve the predefined graphs."));
        }

        let predefined_graphs = first_app_property
            .unwrap()
            ._ten
            .and_then(|p| p.predefined_graphs)
            .ok_or_else(|| {
                anyhow::anyhow!(
                    "No predefined graph is found in the first app."
                )
            })?;

        if let Some(predefined_graph_name) = &command.predefined_graph_name {
            let predefined_graph = predefined_graphs
                .iter()
                .find(|g| g.name == predefined_graph_name.as_str())
                .ok_or_else(|| {
                    anyhow::anyhow!(
                        "Predefined graph [{}] is not found.",
                        predefined_graph_name
                    )
                })?;
            graphs_to_be_checked.push(predefined_graph.graph.clone());
        } else {
            for predefined_graph in predefined_graphs {
                graphs_to_be_checked.push(predefined_graph.graph.clone());
            }
        }
    }

    Ok(graphs_to_be_checked)
}

fn display_error(e: &anyhow::Error) {
    e.to_string().lines().for_each(|l| {
        println!("  {}", l);
    });
}

pub async fn execute_cmd(
    _tman_config: &TmanConfig,
    command_data: CheckGraphCommand,
) -> Result<()> {
    validate_cmd_args(&command_data)?;

    let existed_pkgs_of_all_apps = get_existed_pkgs_of_all_apps(&command_data)?;
    let graphs = get_graphs_to_be_checked(&command_data)?;

    let mut err_count = 0;

    for (graph_idx, graph) in graphs.iter().enumerate() {
        print!("Checking graph[{}]... ", graph_idx);

        match graph.check(&existed_pkgs_of_all_apps) {
            Ok(_) => println!("{}", Emoji("👍", "Passed")),
            Err(e) => {
                err_count += 1;
                println!("{}  Details:", Emoji("🔴", "Failed"));
                display_error(&e);
                println!();
            }
        }
    }

    println!("All is done.");

    if err_count > 0 {
        Err(anyhow::anyhow!(
            "{}/{} graphs failed.",
            err_count,
            graphs.len()
        ))
    } else {
        Ok(())
    }
}
