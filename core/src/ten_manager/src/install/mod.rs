//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
pub mod installed_paths;
pub mod template;

use std::path::{Path, PathBuf};

use anyhow::Result;
use installed_paths::{
    modify_installed_paths_for_system_package, save_installed_paths,
};
use tempfile::NamedTempFile;

use ten_rust::pkg_info::{pkg_type::PkgType, PkgInfo};

use super::{config::TmanConfig, fs::merge_folders, registry::get_package};
use crate::{
    log::tman_verbose_println, package_file::unzip::extract_and_process_zip,
};

pub struct PkgIdentityMapping {
    pub pkg_type: PkgType,

    pub src_pkg_name: String,
    pub dest_pkg_name: String,
}

pub async fn install_pkg_info(
    tman_config: &TmanConfig,
    pkg_info: &PkgInfo,
    pkg_identity_mappings: &Vec<PkgIdentityMapping>,
    template_ctx: Option<&serde_json::Value>,
    base_dir: &Path,
) -> Result<()> {
    if pkg_info.is_local_installed {
        tman_verbose_println!(
            tman_config,
            "{}:{} has already been installed.\n",
            pkg_info.pkg_type,
            pkg_info.name
        );
        return Ok(());
    }

    let cwd = crate::utils::get_cwd()?;

    let mut found_pkg_identity_mapping = None;
    for pkg_identity_mapping in pkg_identity_mappings {
        if pkg_info.pkg_type == pkg_identity_mapping.pkg_type
            && pkg_info.name == pkg_identity_mapping.src_pkg_name
        {
            found_pkg_identity_mapping = Some(pkg_identity_mapping);
        }
    }

    let path;
    if let Some(found_pkg_identity_mapping) = found_pkg_identity_mapping {
        path = Path::new(&base_dir)
            .join(found_pkg_identity_mapping.dest_pkg_name.clone());
    } else {
        path = PathBuf::from(&base_dir).join(pkg_info.name.clone());
    }

    let output_dir = path.to_string_lossy().to_string();

    let mut temp_file = NamedTempFile::new()?;
    get_package(tman_config, &pkg_info.url, &mut temp_file).await?;

    let mut installed_paths = extract_and_process_zip(
        &temp_file.path().to_string_lossy(),
        &output_dir,
        template_ctx,
    )?;

    // After installation (after decompression), check whether the content of
    // property.json is correct based on the decompressed content.
    ten_rust::pkg_info::property::check_property_json_of_pkg(&output_dir)
        .map_err(|e| {
            anyhow::anyhow!(
                "Failed to check property.json for {}:{}, {}",
                pkg_info.pkg_type,
                pkg_info.name,
                e
            )
        })?;

    // Special handling for the system package.
    //
    // NOTE: This feature is not currently in use, but a command-line argument
    // could be added to copy the lib/ or include/ from the system package to
    // ten_packages/system/. This would reduce the number of rpaths.
    if pkg_info.pkg_type == PkgType::System {
        let inclusions = vec![];
        merge_folders(
            Path::new(&output_dir),
            &PathBuf::from(&cwd).join("ten_packages").join("system"),
            &inclusions,
        )?;
        modify_installed_paths_for_system_package(
            &mut installed_paths,
            &inclusions,
        );
    }

    // base_dir is also an installed_path.
    installed_paths.paths.push(".".to_string());

    tman_verbose_println!(
        tman_config,
        "Install files for {}:{}",
        pkg_info.pkg_type,
        pkg_info.name
    );
    for install_path in &installed_paths.paths {
        tman_verbose_println!(tman_config, "{}", install_path);
    }
    tman_verbose_println!(tman_config, "");

    save_installed_paths(&installed_paths, Path::new(&output_dir))?;

    Ok(())
}
