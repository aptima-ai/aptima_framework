//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
use anyhow::{anyhow, Result};

use super::DevServerState;
use crate::package_info::tman_get_all_existed_pkgs_info_of_app;

pub fn get_all_pkgs(state: &mut DevServerState) -> Result<()> {
    use std::path::PathBuf;

    if state.all_pkgs.is_none() {
        if let Some(base_dir) = &state.base_dir {
            let app_path = PathBuf::from(base_dir);
            match tman_get_all_existed_pkgs_info_of_app(
                &state.tman_config,
                &app_path,
            ) {
                Ok(pkgs) => state.all_pkgs = Some(pkgs),
                Err(err) => {
                    return Err(err);
                }
            }
        } else {
            return Err(anyhow!("Base directory not found"));
        }
    }

    Ok(())
}
