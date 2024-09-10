//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
use std::{
    collections::HashSet,
    fs::{self, File},
    io::Write,
    path::Path,
};

use anyhow::Result;
use serde::{Deserialize, Serialize};
use serde_json::json;

use crate::constants::{
    DOT_TEN_DIR, INSTALLED_PATHS_JSON_FILENAME, INSTALL_PATHS_APP_PREFIX,
    PACKAGE_INFO_DIR_IN_DOT_TEN_DIR,
};

#[derive(Serialize, Deserialize)]
pub struct InstalledPaths {
    pub paths: Vec<String>,
}

pub fn sort_installed_paths(installed_paths: &mut InstalledPaths) {
    installed_paths.paths.sort_by(|a, b| {
        let a_path = Path::new(a);
        let b_path = Path::new(b);

        let a_depth = a_path.components().count();
        let b_depth = b_path.components().count();

        a_depth.cmp(&b_depth).reverse()
    });
}

pub fn modify_installed_paths_for_system_package(
    installed_paths: &mut InstalledPaths,
    inclusions: &[String],
) {
    let inclusions_set: HashSet<String> = inclusions.iter().cloned().collect();

    let processed: Vec<String> = installed_paths
        .paths
        .iter_mut()
        .map(|path| {
            if inclusions_set.contains(path) {
                format!("{}/{}", INSTALL_PATHS_APP_PREFIX, path)
            } else {
                path.clone()
            }
        })
        .collect();

    // Modify the original paths.
    for (i, new_path) in processed.into_iter().enumerate() {
        installed_paths.paths[i] = new_path;
    }
}

pub fn save_installed_paths(
    installed_paths: &InstalledPaths,
    cwd: &Path,
) -> Result<()> {
    let target_dir =
        cwd.join(DOT_TEN_DIR).join(PACKAGE_INFO_DIR_IN_DOT_TEN_DIR);
    let target_file = target_dir.join(INSTALLED_PATHS_JSON_FILENAME);
    fs::create_dir_all(&target_dir)?;

    let json_data = json!(installed_paths.paths);
    let mut file = File::create(target_file)?;
    file.write_all(json_data.to_string().as_bytes())?;

    Ok(())
}
