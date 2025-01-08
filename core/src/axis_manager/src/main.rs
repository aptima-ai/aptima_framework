//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//

// Enable this when development.
// #![allow(dead_code)]

use std::process;

use console::Emoji;
use tokio::runtime::Runtime;

use axis_manager::cmd;
use axis_manager::cmd_line;
use axis_manager::config::TmanConfig;

fn merge(cmd_line: TmanConfig, config_file: TmanConfig) -> TmanConfig {
    TmanConfig {
        registry: config_file.registry,
        config_file: cmd_line.config_file,
        admin_token: cmd_line.admin_token.or(config_file.admin_token),
        user_token: cmd_line.user_token.or(config_file.user_token),
        verbose: cmd_line.verbose,
        assume_yes: cmd_line.assume_yes,
        mi_mode: cmd_line.mi_mode,
    }
}

fn main() {
    let mut tman_config_from_cmd_line = TmanConfig::default();

    let command_data = match cmd_line::parse_cmd(&mut tman_config_from_cmd_line)
    {
        Ok(cmd_data) => cmd_data,
        Err(e) => {
            eprintln!("{}  Error: {}", Emoji("🔴", ":-("), e);
            process::exit(1);
        }
    };

    let tman_config_from_config_file = axis_manager::config::read_config(
        &tman_config_from_cmd_line.config_file,
    );

    let tman_config =
        merge(tman_config_from_cmd_line, tman_config_from_config_file);

    let rt = Runtime::new().unwrap();
    let result = rt.block_on(cmd::execute_cmd(&tman_config, command_data));
    if let Err(e) = result {
        println!("{}  Error: {:?}", Emoji("🔴", ":-("), e);
        process::exit(1);
    }

    process::exit(0);
}
