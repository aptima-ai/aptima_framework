use std::path::PathBuf;

//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
use anyhow::{Context, Result};
use clap::{Arg, ArgMatches, Command};
use serde_json::Value;

use crate::{
    config::TmanConfig, constants::PROPERTY_JSON_FILENAME,
    utils::read_file_to_string,
};

#[derive(Debug)]
pub struct ModifyGraphCommand {
    pub app_dir: String,
    pub predefined_graph_name: String,
    pub modifications: Vec<String>,
}

pub fn create_sub_cmd(_args_cfg: &crate::cmd_line::ArgsCfg) -> Command {
    Command::new("graph")
.about("Modify a specified predefined graph in property.json")
.arg(
    Arg::new("APP_DIR")
        .long("app-dir")
        .help("Specify the app directory")
        .required(true)
        .num_args(1)
)
.arg(
    Arg::new("PREDEFINED_GRAPH_NAME")
        .long("predefined-graph-name")
        .help("Specify the predefined graph name to be modified")
        .required(true)
        .num_args(1)
)
.arg(
    Arg::new("MODIFICATIONS")
        .help("The path=JsonString to modify in the selected graph. E.g. nodes[1].property.a=\"\\\"foo\\\"\"")
        .required(true)
        .num_args(1)
)
}

pub fn parse_sub_cmd(sub_cmd_args: &ArgMatches) -> ModifyGraphCommand {
    let cmd = ModifyGraphCommand {
        app_dir: sub_cmd_args
            .get_one::<String>("APP_DIR")
            .unwrap()
            .to_string(),
        predefined_graph_name: sub_cmd_args
            .get_one::<String>("PREDEFINED_GRAPH_NAME")
            .unwrap()
            .to_string(),
        modifications: sub_cmd_args
            .get_many::<String>("MODIFICATIONS")
            .unwrap_or_default()
            .map(|s| s.to_string())
            .collect(),
    };

    cmd
}

pub async fn execute_cmd(
    _tman_config: &TmanConfig,
    command_data: ModifyGraphCommand,
) -> Result<()> {
    // Find `property.json`.
    let property_file_path =
        PathBuf::from(&command_data.app_dir).join(PROPERTY_JSON_FILENAME);
    let mut property_str = read_file_to_string(&property_file_path)
        .with_context(|| {
            format!("Failed to read file: {:?}", property_file_path)
        })?;

    // Parse `property.json`.
    let mut property_json: Value = serde_json::from_str(&property_str)
        .with_context(|| {
            format!("Failed to parse {} into JSON", PROPERTY_JSON_FILENAME)
        })?;

    // Find the specified predefined_graph.
    let graphs_val = property_json
        .pointer_mut("/_ten/predefined_graphs")
        .ok_or_else(|| {
            anyhow::anyhow!(
                "No predefined_graphs in {}",
                PROPERTY_JSON_FILENAME
            )
        })?;

    let graphs = graphs_val.as_array_mut().ok_or_else(|| {
        anyhow::anyhow!(
            "predefined_graphs in {} is not an array",
            PROPERTY_JSON_FILENAME
        )
    })?;

    let mut target_graph_opt = None;
    for g in graphs.iter_mut() {
        if let Some(name_val) = g.get("name") {
            if name_val == command_data.predefined_graph_name.as_str() {
                target_graph_opt = Some(g);
                break;
            }
        }
    }

    let target_graph = target_graph_opt.ok_or_else(|| {
        anyhow::anyhow!(
            "Predefined graph with name='{}' not found",
            command_data.predefined_graph_name
        )
    })?;

    println!(
        "Successfully modified the graph '{}'",
        command_data.predefined_graph_name
    );
    Ok(())
}
