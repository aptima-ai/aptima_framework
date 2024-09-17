//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
use std::time::Instant;

use anyhow::Result;
use clap::{ArgMatches, Command};
use console::Emoji;
use indicatif::HumanDuration;

use crate::{
    config::TmanConfig,
    log::tman_verbose_println,
    package_file::{create_package_zip_file, get_package_zip_file_name},
};
use ten_rust::pkg_info::get_pkg_info_from_path;

#[derive(Debug)]
pub struct PackageCommand {}

pub fn create_sub_cmd(_args_cfg: &crate::cmd_line::ArgsCfg) -> Command {
    Command::new("package")
        .about("Create a package file. For more detailed usage, run 'package -h'")
        .after_help("Switch to the base directory of the TEN package you want to package, then simply run 'tman package' directly in that directory.")
}

pub fn parse_sub_cmd(
    _sub_cmd_args: &ArgMatches,
) -> crate::cmd::cmd_package::PackageCommand {
    crate::cmd::cmd_package::PackageCommand {}
}

pub async fn execute_cmd(
    tman_config: &TmanConfig,
    command_data: PackageCommand,
) -> Result<()> {
    tman_verbose_println!(tman_config, "Executing package command");
    tman_verbose_println!(tman_config, "{:?}", command_data);

    let started = Instant::now();

    let cwd = crate::utils::get_cwd()?;

    let pkg_info = get_pkg_info_from_path(&cwd)?;
    let output_zip_file_name = get_package_zip_file_name(&pkg_info)?;

    let output_zip_file_path_str =
        create_package_zip_file(tman_config, &output_zip_file_name, &cwd)?;

    println!(
        "{}  Pack package to {:?} in {}",
        Emoji("🏆", ":-)"),
        output_zip_file_path_str,
        HumanDuration(started.elapsed())
    );

    Ok(())
}
