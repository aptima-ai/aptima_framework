//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
use std::{
    fs::{self, remove_file},
    path::Path,
    time::Instant,
};

use anyhow::{bail, Context, Ok, Result};
use clap::{Arg, ArgMatches, Command};
use console::Emoji;
use indicatif::HumanDuration;
use serde_json::from_reader;

use crate::{
    config::TmanConfig,
    constants::{
        DOT_TEN_DIR, INSTALLED_PATHS_JSON_FILENAME, INSTALL_PATHS_APP_PREFIX,
        PACKAGE_INFO_DIR_IN_DOT_TEN_DIR, TEN_PACKAGES_DIR,
    },
    install::installed_paths::InstalledPaths,
    log::tman_verbose_println,
    utils::check_is_app_folder,
};

#[derive(Debug)]
pub struct UninstallCommand {
    pub package_type: String,
    pub package_name: String,
}

pub fn create_sub_cmd(args_cfg: &crate::cmd_line::ArgsCfg) -> Command {
    Command::new("uninstall")
        .about(
            "Uninstall a package. For more detailed usage, run 'uninstall -h'",
        )
        .arg(
            Arg::new("PACKAGE_TYPE")
                .help("The type of the package")
                .value_parser(args_cfg.pkg_type.possible_values.clone())
                .required(true),
        )
        .arg(
            Arg::new("PACKAGE_NAME")
                .help("The name of the package")
                .required(true),
        )
}

pub fn parse_sub_cmd(sub_cmd_args: &ArgMatches) -> UninstallCommand {
    let cmd = UninstallCommand {
        package_type: sub_cmd_args
            .get_one::<String>("PACKAGE_TYPE")
            .expect("PACKAGE_TYPE is a required argument")
            .clone(),
        package_name: sub_cmd_args
            .get_one::<String>("PACKAGE_NAME")
            .expect("PACKAGE_NAME is a required argument")
            .clone(),
    };

    cmd
}

async fn remove_installed_paths(
    cwd: &Path,
    uninstall_cmd: &UninstallCommand,
) -> Result<()> {
    let addon_path = cwd
        .join(TEN_PACKAGES_DIR)
        .join(&uninstall_cmd.package_type)
        .join(&uninstall_cmd.package_name);

    if !addon_path.exists() {
        bail!(
            "{}:{} does not exist.",
            uninstall_cmd.package_type,
            uninstall_cmd.package_name
        );
    }

    let installed_paths_path = addon_path
        .join(DOT_TEN_DIR)
        .join(PACKAGE_INFO_DIR_IN_DOT_TEN_DIR)
        .join(INSTALLED_PATHS_JSON_FILENAME);

    // Read the installed_paths.json file.
    let file = fs::File::open(&installed_paths_path).with_context(|| {
        format!("Failed to open file: {:?}", installed_paths_path)
    })?;

    let installed_paths = InstalledPaths {
        paths: from_reader(file).with_context(|| {
            format!(
                "Failed to read installed paths of {}:{}",
                uninstall_cmd.package_type, uninstall_cmd.package_name
            )
        })?,
    };

    // Remove the installed_paths.json file.
    fs::remove_file(&installed_paths_path)?;

    // Remove the .ten/ directory.
    let ten_path = installed_paths_path.parent().unwrap().parent().unwrap();
    fs::remove_dir_all(ten_path)?;

    // Process each path.
    for path_str in installed_paths.paths {
        let path = if path_str
            .starts_with(&format!("{}/", INSTALL_PATHS_APP_PREFIX))
        {
            let relative_path = path_str
                .trim_start_matches(&format!("{}/", INSTALL_PATHS_APP_PREFIX));
            cwd.join(relative_path)
        } else {
            addon_path.join(path_str)
        };

        match fs::canonicalize(&path) {
            std::result::Result::Ok(canonical_path) => {
                if canonical_path.is_file() {
                    remove_file(&canonical_path)?;
                } else if canonical_path.is_dir()
                    && fs::read_dir(&canonical_path)?.next().is_none()
                {
                    fs::remove_dir(&canonical_path)?;
                }
            }
            Err(e) => {
                println!("Error canonicalizing path: {}", e);
            }
        }
    }

    Ok(())
}

pub async fn execute_cmd(
    tman_config: &TmanConfig,
    command_data: UninstallCommand,
) -> Result<()> {
    tman_verbose_println!(tman_config, "Executing uninstall command");
    tman_verbose_println!(tman_config, "{:?}", command_data);
    tman_verbose_println!(tman_config, "{:?}", tman_config);

    let started = Instant::now();

    let cwd = crate::utils::get_cwd()?;
    check_is_app_folder(&cwd)?;

    remove_installed_paths(&cwd, &command_data).await?;

    println!(
        "{}  Uninstall {}:{} successfully in {}",
        Emoji("🏆", ":-)"),
        command_data.package_type,
        command_data.package_name,
        HumanDuration(started.elapsed())
    );

    Ok(())
}
