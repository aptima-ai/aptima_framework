//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
use std::{
    io::{BufRead, BufReader},
    process::{Command as StdCommand, Stdio},
};

use anyhow::{anyhow, Result};
use clap::{Arg, ArgMatches, Command};

use crate::{
    config::TmanConfig,
    constants::{MANIFEST_JSON_FILENAME, SCRIPTS},
    log::tman_verbose_println,
};

#[derive(Debug)]
pub struct RunCommand {
    pub script_name: String,

    // Stores additional arguments passed after `--`.
    pub extra_args: Vec<String>,
}

pub fn create_sub_cmd(_args_cfg: &crate::cmd_line::ArgsCfg) -> Command {
    Command::new("run")
        .about("Run a custom script from the manifest.json's scripts field")
        .arg(
            Arg::new("SCRIPT_NAME")
                .required(true)
                .help("Which script to run, e.g. 'start' or 'stop'"),
        )
        .arg(
            Arg::new("EXTRA_ARGS")
                .help("Additional arguments to pass to the script")
                .last(true)
                .allow_hyphen_values(true)
                .required(false),
        )
}

pub fn parse_sub_cmd(sub_cmd_args: &ArgMatches) -> RunCommand {
    let script_name = sub_cmd_args
        .get_one::<String>("SCRIPT_NAME")
        .expect("SCRIPT_NAME is required")
        .to_string();

    let extra_args = sub_cmd_args
        .get_many::<String>("EXTRA_ARGS")
        .map(|vals| vals.map(|s| s.to_string()).collect())
        .unwrap_or_default();

    RunCommand {
        script_name,
        extra_args,
    }
}

pub async fn execute_cmd(
    tman_config: &TmanConfig,
    cmd: RunCommand,
) -> Result<()> {
    tman_verbose_println!(tman_config, "Executing run command: {:?}", cmd);

    // Read `manifest.json` in the current working directory.
    let cwd = crate::utils::get_cwd()?;
    let manifest_path = cwd.join(MANIFEST_JSON_FILENAME);
    if !manifest_path.exists() {
        return Err(anyhow!(
            "{} not found in current directory: {}",
            MANIFEST_JSON_FILENAME,
            manifest_path.display()
        ));
    }

    // Parse `manifest.json`.
    let manifest_json_str = crate::utils::read_file_to_string(&manifest_path)
        .map_err(|e| {
        anyhow!("Failed to read {}: {}", MANIFEST_JSON_FILENAME, e)
    })?;
    let manifest_value: serde_json::Value =
        serde_json::from_str(&manifest_json_str).map_err(|e| {
            anyhow!("Failed to parse {}: {}", MANIFEST_JSON_FILENAME, e)
        })?;

    // Get `scripts`.
    let scripts = match manifest_value.get(SCRIPTS) {
        Some(s) => s.clone(),
        None => {
            return Err(anyhow!(
                "No 'scripts' field found in {}",
                manifest_path.display()
            ))
        }
    };

    if !scripts.is_object() {
        return Err(anyhow!(
            "'{}' in {} must be an object",
            SCRIPTS,
            MANIFEST_JSON_FILENAME
        ));
    }

    // Find the specified script_name.
    let scripts_obj = scripts.as_object().unwrap();
    let script_cmd = match scripts_obj.get(&cmd.script_name) {
        Some(v) => v.as_str().ok_or_else(|| {
            anyhow!(
                "The script '{}' is not a string in {}",
                &cmd.script_name,
                MANIFEST_JSON_FILENAME
            )
        })?,
        None => {
            return Err(anyhow!(
                "Script '{}' not found in '{}'",
                &cmd.script_name,
                SCRIPTS
            ))
        }
    };

    // Execute the subprocess.
    tman_verbose_println!(
        tman_config,
        "About to run script: {} -> {}",
        &cmd.script_name,
        script_cmd
    );

    let mut parts = script_cmd.split_whitespace();
    let exec = parts.next().unwrap();
    let mut args: Vec<&str> = parts.collect();

    for arg in &cmd.extra_args {
        args.push(arg);
    }

    let mut child = StdCommand::new(exec)
        .args(&args)
        .stdin(Stdio::null())
        .stdout(Stdio::piped())
        .stderr(Stdio::piped())
        .spawn()
        .map_err(|e| anyhow!("Failed to spawn subprocess: {}", e))?;

    // Get stdout.
    if let Some(stdout) = child.stdout.take() {
        let reader = BufReader::new(stdout);
        tokio::spawn(async move {
            for line in reader.lines().map_while(Result::ok) {
                println!("{}", line);
            }
        });
    }

    // Get stderr.
    if let Some(stderr) = child.stderr.take() {
        let reader = BufReader::new(stderr);
        tokio::spawn(async move {
            for line in reader.lines().map_while(Result::ok) {
                eprintln!("{}", line);
            }
        });
    }

    // Wait for the subprocess to exit.
    let status = child
        .wait()
        .map_err(|e| anyhow!("Failed to wait for the subprocess: {}", e))?;

    if !status.success() {
        return Err(anyhow!(
            "Script '{}' exited with non-zero code: {:?}",
            &cmd.script_name,
            status.code()
        ));
    }

    Ok(())
}
