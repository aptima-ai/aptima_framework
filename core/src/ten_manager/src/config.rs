//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
use std::collections::HashMap;
use std::fs;
use std::path::PathBuf;

use serde::{Deserialize, Serialize};

use super::constants::DEFAULT_REGISTRY;

#[derive(Serialize, Deserialize, Debug)]
pub struct Registry {
    pub index: String,
}

#[derive(Serialize, Deserialize, Debug, Default)]
pub struct TmanConfigFile {
    pub registry: HashMap<String, Registry>,

    pub admin_token: Option<String>,
    pub user_token: Option<String>,
}

#[derive(Serialize, Deserialize, Debug, Default)]
pub struct TmanConfig {
    pub registry: HashMap<String, Registry>,

    pub config_file: Option<String>,
    pub admin_token: Option<String>,
    pub user_token: Option<String>,

    pub verbose: bool,
    pub assume_yes: bool,

    pub mi_mode: bool,
}

// Determine the config file path based on the platform.
fn get_default_config_path() -> PathBuf {
    match dirs::home_dir() {
        Some(mut path) => {
            if cfg!(target_os = "windows") {
                // Windows path.
                path.push("AppData\\Roaming\\tman\\config.json");
            } else {
                // Linux and macOS path
                path.push(".tman");
                path.push("config.json");
            }
            path
        }
        None => panic!("Cannot determine home directory."),
    }
}

// Read the configuration from the specified path.
pub fn read_config(config_file_path: &Option<String>) -> TmanConfig {
    let config_path = match config_file_path {
        Some(path) => PathBuf::from(path),
        None => get_default_config_path(),
    };

    let config_data = fs::read_to_string(config_path.clone());

    let mut config_file_content: TmanConfigFile = if config_path.exists() {
        match config_data {
            Ok(data) => serde_json::from_str(&data).unwrap_or_else(
                |e: serde_json::Error| {
                    panic!("Failed to parse config file: {}", e);
                },
            ),
            Err(e) => {
                panic!("Failed to parse config file: {}", e);
            }
        }
    } else {
        TmanConfigFile::default()
    };

    // Ensure there is a default registry entry if it does not exist.
    config_file_content
        .registry
        .entry("default".to_string())
        .or_insert(Registry {
            index: DEFAULT_REGISTRY.to_string(),
        });

    TmanConfig {
        registry: config_file_content.registry,
        config_file: Some(config_path.to_string_lossy().to_string()),
        admin_token: config_file_content.admin_token,
        user_token: config_file_content.user_token,
        verbose: false,
        assume_yes: false,
        mi_mode: false,
    }
}
