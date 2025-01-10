//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
use anyhow::{anyhow, Result};

use super::DesignerState;
use crate::package_info::tman_get_all_installed_pkgs_info_of_app;

pub fn get_all_pkgs(state: &mut DesignerState) -> Result<()> {
    use std::path::PathBuf;

    if state.all_pkgs.is_none() {
        if let Some(base_dir) = &state.base_dir {
            let app_path = PathBuf::from(base_dir);
            match tman_get_all_installed_pkgs_info_of_app(
                &state.tman_config,
                &app_path,
            ) {
                Ok(pkgs) => state.all_pkgs = Some(pkgs),
                Err(err) => {
                    return Err(err);
                }
            }
        } else {
            return Err(anyhow!("Base directory is not set"));
        }
    }

    Ok(())
}
