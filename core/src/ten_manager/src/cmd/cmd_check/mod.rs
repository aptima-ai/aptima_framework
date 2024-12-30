//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
pub mod cmd_check_graph;
pub mod cmd_check_manifest_json;
pub mod cmd_check_property_json;

use anyhow::Result;
use clap::{ArgMatches, Command};

use crate::config::TmanConfig;

#[derive(Debug)]
pub enum CheckCommandData {
    CheckGraph(cmd_check_graph::CheckGraphCommand),
    CheckManifestJson(cmd_check_manifest_json::CheckManifestJsonCommand),
    CheckPropertyJson(cmd_check_property_json::CheckPropertyJsonCommand),
}

pub fn create_sub_cmd(args_cfg: &crate::cmd_line::ArgsCfg) -> Command {
    Command::new("check")
        .about("Check various consistency validations")
        .subcommand_required(true)
        .arg_required_else_help(true)
        .subcommand(crate::cmd::cmd_check::cmd_check_graph::create_sub_cmd(
            args_cfg,
        ))
        .subcommand(
            crate::cmd::cmd_check::cmd_check_manifest_json::create_sub_cmd(
                args_cfg,
            ),
        )
        .subcommand(
            crate::cmd::cmd_check::cmd_check_property_json::create_sub_cmd(
                args_cfg,
            ),
        )
}

pub fn parse_sub_cmd(sub_cmd_args: &ArgMatches) -> Result<CheckCommandData> {
    let command_data = match sub_cmd_args.subcommand() {
        Some(("graph", graph_cmd_args)) => CheckCommandData::CheckGraph(
            crate::cmd::cmd_check::cmd_check_graph::parse_sub_cmd(
                graph_cmd_args,
            )?,
        ),
        Some(("manifest-json", manifest_json_cmd_args)) => {
            CheckCommandData::CheckManifestJson(
                crate::cmd::cmd_check::cmd_check_manifest_json::parse_sub_cmd(
                    manifest_json_cmd_args,
                )?,
            )
        }
        Some(("property-json", property_json_cmd_args)) => {
            CheckCommandData::CheckPropertyJson(
                crate::cmd::cmd_check::cmd_check_property_json::parse_sub_cmd(
                    property_json_cmd_args,
                )?,
            )
        }

        _ => unreachable!("Command not found"),
    };

    Ok(command_data)
}

pub async fn execute_cmd(
    tman_config: &TmanConfig,
    command_data: CheckCommandData,
) -> Result<()> {
    match command_data {
        CheckCommandData::CheckGraph(cmd) => {
            crate::cmd::cmd_check::cmd_check_graph::execute_cmd(
                tman_config,
                cmd,
            )
            .await
        }
        CheckCommandData::CheckManifestJson(cmd) => {
            crate::cmd::cmd_check::cmd_check_manifest_json::execute_cmd(
                tman_config,
                cmd,
            )
            .await
        }
        CheckCommandData::CheckPropertyJson(cmd) => {
            crate::cmd::cmd_check::cmd_check_property_json::execute_cmd(
                tman_config,
                cmd,
            )
            .await
        }
    }
}
