//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
use std::{
    path::Path,
    sync::{Arc, RwLock},
};

use actix_cors::Cors;
use actix_web::{http::header, web, App, HttpServer};
use anyhow::{Ok, Result};
use clap::{value_parser, Arg, ArgMatches, Command};
use console::Emoji;

use crate::{
    config::TmanConfig,
    dev_server::{configure_routes, DevServerState},
    error::TmanError,
    log::tman_verbose_println,
    utils::{check_is_app_folder, get_cwd},
};

#[derive(Debug)]
pub struct DevServerCommand {
    pub ip_address: String,
    pub port: u16,
    pub base_dir: Option<String>,
}

pub fn create_sub_cmd(_args_cfg: &crate::cmd_line::ArgsCfg) -> Command {
    Command::new("dev-server")
        .about(
            "Install a package. For more detailed usage, run 'dev-server -h'",
        )
        .arg(
            Arg::new("IP_ADDRESS")
                .long("ip")
                .help("Sets the IP address")
                .default_value("0.0.0.0"),
        )
        .arg(
            Arg::new("PORT")
                .long("port")
                .help("Sets the port number")
                .value_parser(value_parser!(u16).range(0..=65535))
                // Why we choose 49483?
                //
                // 'r' -> ascii value: 114
                // 't' -> ascii value: 116
                // 'e' -> ascii value: 101
                //
                // ascii_sum = 114 + 116 + 101 = 331
                //
                // The range of dynamic or private ports is 49152-65535.
                // range_size = 65535 - 49152 + 1 = 16384
                // port_number = (331 % 16384) + 49152
                //             = 338 + 49152
                //             = 49483
                .default_value("49483"),
        )
        .arg(
            Arg::new("BASE_DIR")
                .long("base-dir")
                .help("The base directory")
                .required(false),
        )
}

pub fn parse_sub_cmd(
    sub_cmd_args: &ArgMatches,
) -> crate::cmd::cmd_dev_server::DevServerCommand {
    let cmd = crate::cmd::cmd_dev_server::DevServerCommand {
        ip_address: sub_cmd_args
            .get_one::<String>("IP_ADDRESS")
            .unwrap()
            .to_string(),
        port: *sub_cmd_args.get_one::<u16>("PORT").unwrap(),
        base_dir: sub_cmd_args.get_one::<String>("BASE_DIR").cloned(),
    };

    cmd
}

pub async fn execute_cmd(
    tman_config: &TmanConfig,
    command_data: DevServerCommand,
) -> Result<()> {
    tman_verbose_println!(tman_config, "Executing dev-server command");
    tman_verbose_println!(tman_config, "{:?}", command_data);
    tman_verbose_println!(tman_config, "{:?}", tman_config);

    let cwd = get_cwd()?;
    let base_dir = command_data
        .base_dir
        .unwrap_or_else(|| cwd.to_str().unwrap().to_string());

    let state = Arc::new(RwLock::new(DevServerState {
        base_dir: Some(base_dir.clone()),
        all_pkgs: None,
        tman_config: TmanConfig::default(),
    }));

    if let Err(e) = check_is_app_folder(Path::new(&base_dir)) {
        return Err(TmanError::Custom(format!(
            "base_dir '{}' is not app base directory: {}",
            base_dir, e
        ))
        .into());
    }

    let server = HttpServer::new(move || {
        let state = web::Data::new(state.clone());

        let cors = Cors::default()
            .allow_any_origin()
            .allowed_methods(vec!["GET", "POST", "PUT", "DELETE"])
            .allowed_headers(vec![header::AUTHORIZATION, header::ACCEPT])
            .allowed_header(header::CONTENT_TYPE)
            .max_age(3600);

        App::new()
            .app_data(state.clone())
            .wrap(cors)
            .configure(|cfg| configure_routes(cfg, state.clone()))
    });

    let bind_address =
        format!("{}:{}", command_data.ip_address, command_data.port);

    println!(
        "{}  Starting server at http://{}",
        Emoji("🏆", ":-)"),
        bind_address,
    );

    server.bind(&bind_address)?.run().await?;

    Ok(())
}
