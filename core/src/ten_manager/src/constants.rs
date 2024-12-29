//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//

// Go to https://ten-console.theten.ai/ to get user token.
pub const DEFAULT_REGISTRY: &str =
    "https://ten-store.theten.ai/api/ten-cloud-store/v1/packages";

// This comment is just for reference.
//
// pub const ADMIN_TOKEN: &str =
//     "cnRlX3N0b3JlOjNjYzc2ODgxYjExNjQ0NTM4ZDQ5YmNmNmNlYjQ2Yjk0";

pub const DOT_TEN_DIR: &str = ".ten";
pub const APP_DIR_IN_DOT_TEN_DIR: &str = "app";
pub const INSTALLED_PATHS_JSON_FILENAME: &str = "installed_paths.json";
pub const PACKAGE_INFO_DIR_IN_DOT_TEN_DIR: &str = "package_info";
pub const PACKAGE_DIR_IN_DOT_TEN_DIR: &str = "package";

pub const LIB_DIR: &str = "lib";
pub const INCLUDE_DIR: &str = "include";

pub const APP_PKG_TYPE: &str = "app";

pub const MANIFEST_JSON_FILENAME: &str = "manifest.json";
pub const MANIFEST_LOCK_JSON_FILENAME: &str = "manifest-lock.json";
pub const PROPERTY_JSON_FILENAME: &str = "property.json";
pub const SCRIPTS: &str = "scripts";

pub const INSTALL_PATHS_APP_PREFIX: &str = "@app";

pub const TEN_PACKAGE_FILE_EXTENSION: &str = "tpkg";

// Maximum number of retry attempts.
pub const REMOTE_REGISTRY_MAX_RETRIES: u32 = 30;
// Delay between retries in milliseconds.
pub const REMOTE_REGISTRY_RETRY_DELAY_MS: u64 = 500;
// Timeout duration for requests in seconds.
pub const REMOTE_REGISTRY_REQUEST_TIMEOUT_SECS: u64 = 10;
