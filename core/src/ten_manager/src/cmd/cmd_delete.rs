//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
use std::str::FromStr;
use std::time::Instant;

use anyhow::Result;
use clap::{Arg, ArgMatches, Command};
use console::Emoji;
use indicatif::HumanDuration;
use semver::Version;

use crate::log::tman_verbose_println;
use crate::{config::TmanConfig, registry::delete_package};
use ten_rust::pkg_info::pkg_identity::PkgIdentity;
use ten_rust::pkg_info::pkg_type::PkgType;

#[derive(Debug)]
pub struct DeleteCommand {
    pub package_type: String,
    pub package_name: String,
    pub version: String,
    pub hash: String,
}

pub fn create_sub_cmd(args_cfg: &crate::cmd_line::ArgsCfg) -> Command {
    Command::new("delete")
        .about("Delete a package. For more detailed usage, run 'delete -h'")
        .after_help("This is a hidden privileged command, use it with caution.")
        .hide(true)
        .arg(
            Arg::new("PACKAGE_TYPE")
                .help("The type of the package")
                .value_parser(args_cfg.pkg_type.possible_values.clone())
                .required(true)
                // If PACKAGE_TYPE is provided, PACKAGE_NAME must be too.
                .requires("PACKAGE_NAME"),
        )
        .arg(
            Arg::new("PACKAGE_NAME")
                .help("The name of the package")
                .required(true),
        )
        .arg(
            Arg::new("VERSION")
                .help("The name of the package")
                .required(true),
        )
        .arg(
            Arg::new("HASH")
                .help("The hash of the package")
                .required(true),
        )
}

pub fn parse_sub_cmd(
    sub_cmd_args: &ArgMatches,
) -> crate::cmd::cmd_delete::DeleteCommand {
    crate::cmd::cmd_delete::DeleteCommand {
        package_type: sub_cmd_args
            .get_one::<String>("PACKAGE_TYPE")
            .cloned()
            .unwrap(),
        package_name: sub_cmd_args
            .get_one::<String>("PACKAGE_NAME")
            .cloned()
            .unwrap(),
        version: sub_cmd_args.get_one::<String>("VERSION").cloned().unwrap(),
        hash: sub_cmd_args.get_one::<String>("HASH").cloned().unwrap(),
    }
}

pub async fn execute_cmd(
    tman_config: &TmanConfig,
    command_data: DeleteCommand,
) -> Result<()> {
    tman_verbose_println!(tman_config, "Executing delete command");
    tman_verbose_println!(tman_config, "{:?}", command_data);

    let started = Instant::now();

    delete_package(
        tman_config,
        &PkgIdentity {
            pkg_type: PkgType::from_str(&command_data.package_type)?,
            name: command_data.package_name,
        },
        &Version::parse(&command_data.version)?,
        &command_data.hash,
    )
    .await?;

    println!(
        "{}  Delete successfully in {}",
        Emoji("🏆", ":-)"),
        HumanDuration(started.elapsed())
    );

    Ok(())
}
