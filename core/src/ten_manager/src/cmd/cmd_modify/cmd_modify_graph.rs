use std::{fs, path::PathBuf};

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
    pub modification: String,
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
            Arg::new("MODIFICATION")
                .long("modification")
                .short('m')
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
        modification: sub_cmd_args
            .get_one::<String>("MODIFICATION")
            .unwrap()
            .to_string(),
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
    let property_str =
        read_file_to_string(&property_file_path).with_context(|| {
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

    // Handle modification. The format is `path=JsonString`.
    // e.g. nodes[1].property.a="\"foo\""
    // Split it into "nodes[1].property.a" 與 "\"foo\""
    let mut iter = command_data.modification.splitn(2, '=');
    let json_path = iter
        .next()
        .ok_or_else(|| anyhow::anyhow!("Invalid modification format"))?
        .trim();
    let raw_value_str = iter
        .next()
        .ok_or_else(|| anyhow::anyhow!("No '=' found in modification"))?
        .trim();

    // raw_value_str should be a valid JSON string, e.g., "\"foo\"" =>
    // After parsing, it will become "foo"
    let new_value: Value =
        serde_json::from_str(raw_value_str).with_context(|| {
            format!("Invalid JSON for the new value: {}", raw_value_str)
        })?;

    // Replace this `new_value` into target_graph.
    apply_json_patch(target_graph, json_path, new_value)?;

    // Serialize and write back to property.json.
    let new_property_str = serde_json::to_string_pretty(&property_json)?;
    fs::write(&property_file_path, new_property_str)?;

    println!(
        "Successfully modified the graph '{}'",
        command_data.predefined_graph_name
    );
    Ok(())
}

/// Based on user input like "nodes[1].property.a", locate the corresponding
/// position in `json_obj` and write `new_value` to it.
fn apply_json_patch(
    json_obj: &mut Value,
    path_expr: &str,
    new_value: Value,
) -> Result<()> {
    // Ex: nodes[1].property.a => ["nodes[1]", "property", "a"]
    let segments = path_expr.split('.').collect::<Vec<_>>();

    let mut current_val = json_obj;

    for (i, seg) in segments.iter().enumerate() {
        // Check if an array index is included.
        if let Some(idx_start) = seg.find('[') {
            // Ex, if seg="nodes[1]", then key="nodes" and arr_idx=1
            let key_part = &seg[..idx_start];
            let idx_part = &seg[idx_start + 1..seg.len() - 1];
            let arr_idx: usize = idx_part.parse()?;

            // First, find key_part in the current layer.
            current_val = current_val
                .get_mut(key_part)
                .ok_or_else(|| anyhow::anyhow!("Key {} not found", key_part))?;

            // Then, access the array.
            let arr = current_val.as_array_mut().ok_or_else(|| {
                anyhow::anyhow!("Value of key {} is not array", key_part)
            })?;

            if arr_idx >= arr.len() {
                return Err(anyhow::anyhow!(
                    "Array index out of range: {}",
                    arr_idx
                ));
            }
            current_val = &mut arr[arr_idx];
        } else {
            // Regular object key. If it's the last segment, assign the value.
            if i == segments.len() - 1 {
                // Reached the end.
                current_val
                    .as_object_mut()
                    .ok_or_else(|| {
                        anyhow::anyhow!("Not an object at segment: {}", seg)
                    })?
                    .insert(seg.to_string(), new_value);
                return Ok(());
            } else {
                // Not the last segment, continue traversing.
                current_val = current_val
                    .get_mut(*seg)
                    .ok_or_else(|| anyhow::anyhow!("Key {} not found", seg))?;
            }
        }
    }

    // If `path_expr` is empty, theoretically the loop won't be entered, and
    // this case is not supported.
    Err(anyhow::anyhow!("Invalid path expression: '{}'", path_expr))
}
