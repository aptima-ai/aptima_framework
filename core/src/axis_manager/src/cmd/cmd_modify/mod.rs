//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
mod cmd_modify_graph;
mod jq_util;

use anyhow::Result;
use clap::{ArgMatches, Command};

use crate::config::TmanConfig;

#[derive(Debug)]
pub enum ModifyCommandData {
    ModifyGraph(cmd_modify_graph::ModifyGraphCommand),
}

pub fn create_sub_cmd(args_cfg: &crate::cmd_line::ArgsCfg) -> Command {
    Command::new("modify")
        .about("Modify something in the TEN framework")
        .subcommand_required(true)
        .arg_required_else_help(true)
        .subcommand(crate::cmd::cmd_modify::cmd_modify_graph::create_sub_cmd(
            args_cfg,
        ))
}

pub fn parse_sub_cmd(sub_cmd_args: &ArgMatches) -> Result<ModifyCommandData> {
    let command_data = match sub_cmd_args.subcommand() {
        Some(("graph", graph_cmd_args)) => ModifyCommandData::ModifyGraph(
            crate::cmd::cmd_modify::cmd_modify_graph::parse_sub_cmd(
                graph_cmd_args,
            )?,
        ),

        _ => unreachable!("Command not found"),
    };

    Ok(command_data)
}

pub async fn execute_cmd(
    tman_config: &TmanConfig,
    command_data: ModifyCommandData,
) -> Result<()> {
    match command_data {
        ModifyCommandData::ModifyGraph(cmd) => {
            crate::cmd::cmd_modify::cmd_modify_graph::execute_cmd(
                tman_config,
                cmd,
            )
            .await
        }
    }
}
