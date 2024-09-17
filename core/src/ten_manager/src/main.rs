//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//

// Enable this when development.
// #![allow(dead_code)]

pub mod cmd;
pub mod cmd_line;
pub mod config;
pub mod constants;
mod dep_and_candidate;
mod dev_server;
mod error;
mod fs;
mod install;
mod log;
mod manifest_lock;
mod package_file;
mod package_info;
mod registry;
mod solver;
mod utils;
mod version;

use std::process;

use console::Emoji;
use tokio::runtime::Runtime;

use config::TmanConfig;

#[cfg(not(target_os = "windows"))]
use mimalloc::MiMalloc;

// TODO(Wei): When adding a URL route with variables (e.g., /api/{name}) in
// actix-web, using the default allocator can lead to a memory leak. According
// to the information in the internet, this leak is likely a false positive and
// may be related to the caching mechanism in actix-web. However, if we use
// mimalloc or jemalloc, there won't be any leak. Use this method to avoid the
// issue for now and study it in more detail in the future.
//
// Refer to the following posts:
// https://github.com/hyperium/hyper/issues/1790#issuecomment-2170644852
// https://github.com/actix/actix-web/issues/1780
// https://news.ycombinator.com/item?id=21962195
#[cfg(not(target_os = "windows"))]
#[global_allocator]
static GLOBAL: MiMalloc = MiMalloc;

fn merge(cmd_line: TmanConfig, config_file: TmanConfig) -> TmanConfig {
    TmanConfig {
        registry: config_file.registry,
        config_file: cmd_line.config_file,
        admin_token: cmd_line.admin_token.or(config_file.admin_token),
        user_token: cmd_line.user_token.or(config_file.user_token),
        mi_mode: cmd_line.mi_mode,
        verbose: cmd_line.verbose,
    }
}

fn main() {
    let mut tman_config_from_cmd_line = TmanConfig::default();
    let command_data = cmd_line::parse_cmd(&mut tman_config_from_cmd_line);

    let tman_config_from_config_file =
        crate::config::read_config(&tman_config_from_cmd_line.config_file);

    let tman_config =
        merge(tman_config_from_cmd_line, tman_config_from_config_file);

    let rt = Runtime::new().unwrap();
    let result = rt.block_on(cmd::execute_cmd(&tman_config, command_data));
    if let Err(e) = result {
        println!("{}  Error: {:?}", Emoji("💔", ":-("), e);

        process::exit(1);
    }

    process::exit(0);
}
