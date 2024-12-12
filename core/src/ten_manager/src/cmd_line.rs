//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
use clap::{Arg, Command};

use crate::version::VERSION;

pub struct ArgCfg {
    pub possible_values: Vec<&'static str>,
}

pub struct ArgsCfg {
    pub pkg_type: ArgCfg,
    pub os: ArgCfg,
    pub arch: ArgCfg,
    pub build_type: ArgCfg,
    pub _language: ArgCfg,
}

fn get_args_cfg() -> ArgsCfg {
    ArgsCfg {
        pkg_type: ArgCfg {
            possible_values: [
                "system",
                "protocol",
                "extension",
                "extension_group",
                "app",
            ]
            .to_vec(),
        },
        os: ArgCfg {
            possible_values: ["linux", "mac", "win"].to_vec(),
        },
        arch: ArgCfg {
            possible_values: ["x86", "x64", "arm", "arm64"].to_vec(),
        },
        build_type: ArgCfg {
            possible_values: ["debug", "release"].to_vec(),
        },
        _language: ArgCfg {
            possible_values: ["cpp", "go"].to_vec(),
        },
    }
}

fn create_cmd() -> clap::ArgMatches {
    let args_cfg = get_args_cfg();

    Command::new("tman")
        .about("TEN manager")
        .version(VERSION)
        .subcommand_required(true)
        .arg_required_else_help(true)
        .arg(
            Arg::new("CONFIG_FILE")
                .long("config-file")
                .short('c')
                .help("The location of config.json")
                .default_value(None),
        )
        .arg(
            Arg::new("ADMIN_TOKEN")
                .long("admin-token")
                .help("The administration token")
                .default_value(None)
                .hide(true),
        )
        .arg(
            Arg::new("USER_TOKEN")
                .long("user-token")
                .help("The user token")
                .default_value(None),
        )
        .arg(
            Arg::new("VERBOSE")
                .long("verbose")
                .help("Enable verbose output")
                .action(clap::ArgAction::SetTrue),
        )
        .arg(
            Arg::new("ASSUME_YES")
                .short('y')
                .long("yes")
                .help("Automatically answer 'yes' to all prompts")
                .action(clap::ArgAction::SetTrue),
        )
        .arg(
            Arg::new("MI")
                .long("mi")
                .help("Machine interface mode")
                .action(clap::ArgAction::SetTrue)
                .hide(true),
        )
        .subcommand(crate::cmd::cmd_install::create_sub_cmd(&args_cfg))
        .subcommand(crate::cmd::cmd_uninstall::create_sub_cmd(&args_cfg))
        .subcommand(crate::cmd::cmd_package::create_sub_cmd(&args_cfg))
        .subcommand(crate::cmd::cmd_publish::create_sub_cmd(&args_cfg))
        .subcommand(crate::cmd::cmd_delete::create_sub_cmd(&args_cfg))
        .subcommand(crate::cmd::cmd_dev_server::create_sub_cmd(&args_cfg))
        .subcommand(crate::cmd::cmd_check::create_sub_cmd(&args_cfg))
        .get_matches()
}

pub fn parse_cmd(
    tman_config: &mut crate::config::TmanConfig,
) -> crate::cmd::CommandData {
    let matches = create_cmd();

    tman_config.config_file = matches.get_one::<String>("CONFIG_FILE").cloned();
    tman_config.admin_token = matches.get_one::<String>("ADMIN_TOKEN").cloned();
    tman_config.user_token = matches.get_one::<String>("USER_TOKEN").cloned();
    tman_config.verbose = matches.get_flag("VERBOSE");
    tman_config.assume_yes = matches.get_flag("ASSUME_YES");
    tman_config.mi_mode = matches.get_flag("MI");

    match matches.subcommand() {
        Some(("install", sub_cmd_args)) => crate::cmd::CommandData::Install(
            crate::cmd::cmd_install::parse_sub_cmd(sub_cmd_args),
        ),
        Some(("uninstall", sub_cmd_args)) => {
            crate::cmd::CommandData::Uninstall(
                crate::cmd::cmd_uninstall::parse_sub_cmd(sub_cmd_args),
            )
        }
        Some(("package", sub_cmd_args)) => crate::cmd::CommandData::Package(
            crate::cmd::cmd_package::parse_sub_cmd(sub_cmd_args),
        ),
        Some(("publish", sub_cmd_args)) => crate::cmd::CommandData::Publish(
            crate::cmd::cmd_publish::parse_sub_cmd(sub_cmd_args),
        ),
        Some(("delete", sub_cmd_args)) => crate::cmd::CommandData::Delete(
            crate::cmd::cmd_delete::parse_sub_cmd(sub_cmd_args),
        ),
        Some(("dev-server", sub_cmd_args)) => {
            crate::cmd::CommandData::DevServer(
                crate::cmd::cmd_dev_server::parse_sub_cmd(sub_cmd_args),
            )
        }
        Some(("check", sub_cmd_args)) => crate::cmd::CommandData::Check(
            crate::cmd::cmd_check::parse_sub_cmd(sub_cmd_args),
        ),
        _ => unreachable!("Command not found"),
    }
}
