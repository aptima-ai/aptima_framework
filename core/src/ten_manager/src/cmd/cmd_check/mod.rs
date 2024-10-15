//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
pub mod cmd_check_graph;

use anyhow::Result;
use clap::{ArgMatches, Command};

use crate::config::TmanConfig;

#[derive(Debug)]
pub enum CheckCommandData {
    CheckGraph(crate::cmd::cmd_check::cmd_check_graph::CheckGraphCommand),
}

pub fn create_sub_cmd(args_cfg: &crate::cmd_line::ArgsCfg) -> Command {
    Command::new("check")
        .about("Check various consistency validations. For more detailed usage, run 'check -h'")
        .subcommand_required(true)
        .arg_required_else_help(true)
        .subcommand(crate::cmd::cmd_check::cmd_check_graph::create_sub_cmd(
            args_cfg,
        ))
}

pub fn parse_sub_cmd(sub_cmd_args: &ArgMatches) -> CheckCommandData {
    match sub_cmd_args.subcommand() {
        Some(("graph", graph_cmd_args)) => CheckCommandData::CheckGraph(
            crate::cmd::cmd_check::cmd_check_graph::parse_sub_cmd(
                graph_cmd_args,
            ),
        ),

        _ => unreachable!("Command not found"),
    }
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
    }
}
