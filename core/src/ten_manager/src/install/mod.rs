//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
pub mod installed_paths;
pub mod template;

use std::{
    fs,
    path::{Path, PathBuf},
};

use anyhow::{Context, Result};
use tempfile::NamedTempFile;

use ten_rust::pkg_info::{pkg_type::PkgType, PkgInfo};

use super::{config::TmanConfig, registry::get_package};
use crate::{
    cmd::cmd_install::{InstallCommand, LocalInstallMode},
    fs::copy_folder_recursively,
    log::tman_verbose_println,
    package_file::unzip::extract_and_process_zip,
};
use installed_paths::save_installed_paths;

pub struct PkgIdentityMapping {
    pub pkg_type: PkgType,

    pub src_pkg_name: String,
    pub dest_pkg_name: String,
}

fn install_local_dependency_pkg_info(
    command_data: &InstallCommand,
    pkg_info: &PkgInfo,
    dest_dir_path: &String,
) -> Result<()> {
    assert!(
        pkg_info.local_dependency_path.is_some(),
        "Should not happen.",
    );

    let src_path = pkg_info.local_dependency_path.as_ref().unwrap();
    let src_base_dir =
        pkg_info.local_dependency_base_dir.as_deref().unwrap_or("");

    let src_dir_path = Path::new(&src_base_dir)
        .join(src_path)
        .canonicalize()
        .with_context(|| {
        format!(
            "Failed to canonicalize path: {} + {}",
            src_base_dir, src_path
        )
    })?;

    let src_dir_path_metadata = fs::metadata(&src_dir_path)
        .expect("Failed to get metadata for src_path");
    assert!(
        src_dir_path_metadata.is_dir(),
        "Source path must be a directory."
    );

    match command_data.local_install_mode {
        LocalInstallMode::Invalid => panic!("Should not happen."),
        LocalInstallMode::Copy => {
            copy_folder_recursively(
                &src_dir_path.to_string_lossy().to_string(),
                dest_dir_path,
            )?;
        }
        LocalInstallMode::Link => {
            #[cfg(unix)]
            {
                std::os::unix::fs::symlink(src_dir_path, dest_dir_path)
                    .map_err(|e| {
                        anyhow::anyhow!("Failed to create symlink: {}", e)
                    })?;
            }

            #[cfg(windows)]
            {
                std::os::windows::fs::symlink_dir(src_dir_path, &dest_dir_path)
                    .map_err(|e| {
                        anyhow::anyhow!(
                            "Failed to create directory symlink: {}",
                            e
                        )
                    })?;
            }
        }
    }

    Ok(())
}

async fn install_non_local_dependency_pkg_info(
    tman_config: &TmanConfig,
    pkg_info: &PkgInfo,
    template_ctx: Option<&serde_json::Value>,
    dest_dir_path: &String,
) -> Result<()> {
    let mut temp_file = NamedTempFile::new()?;
    get_package(tman_config, &pkg_info.url, &mut temp_file).await?;

    let mut installed_paths = extract_and_process_zip(
        &temp_file.path().to_string_lossy(),
        dest_dir_path,
        template_ctx,
    )?;

    // After installation (after decompression), check whether the content
    // of property.json is correct based on the decompressed
    // content.
    ten_rust::pkg_info::property::check_property_json_of_pkg(dest_dir_path)
        .map_err(|e| {
            anyhow::anyhow!(
                "Failed to check property.json for {}:{}, {}",
                pkg_info.basic_info.type_and_name.pkg_type,
                pkg_info.basic_info.type_and_name.name,
                e
            )
        })?;

    // base_dir is also an installed_path.
    installed_paths.paths.push(".".to_string());

    tman_verbose_println!(
        tman_config,
        "Install files for {}:{}",
        pkg_info.basic_info.type_and_name.pkg_type,
        pkg_info.basic_info.type_and_name.name
    );
    for install_path in &installed_paths.paths {
        tman_verbose_println!(tman_config, "{}", install_path);
    }
    tman_verbose_println!(tman_config, "");

    save_installed_paths(&installed_paths, Path::new(&dest_dir_path))?;

    Ok(())
}

pub async fn install_pkg_info(
    tman_config: &TmanConfig,
    command_data: &InstallCommand,
    pkg_info: &PkgInfo,
    pkg_identity_mappings: &Vec<PkgIdentityMapping>,
    template_ctx: Option<&serde_json::Value>,
    base_dir: &Path,
) -> Result<()> {
    if pkg_info.is_installed {
        tman_verbose_println!(
            tman_config,
            "{}:{} has already been installed.\n",
            pkg_info.basic_info.type_and_name.pkg_type,
            pkg_info.basic_info.type_and_name.name
        );
        return Ok(());
    }

    let mut found_pkg_identity_mapping = None;
    for pkg_identity_mapping in pkg_identity_mappings {
        if pkg_info.basic_info.type_and_name.pkg_type
            == pkg_identity_mapping.pkg_type
            && pkg_info.basic_info.type_and_name.name
                == pkg_identity_mapping.src_pkg_name
        {
            found_pkg_identity_mapping = Some(pkg_identity_mapping);
        }
    }

    let target_path;
    if let Some(found_pkg_identity_mapping) = found_pkg_identity_mapping {
        target_path = Path::new(&base_dir)
            .join(found_pkg_identity_mapping.dest_pkg_name.clone());
    } else {
        target_path = PathBuf::from(&base_dir)
            .join(pkg_info.basic_info.type_and_name.name.clone());
    }

    let dest_dir_path = target_path.to_string_lossy().to_string();

    if pkg_info.is_local_dependency {
        install_local_dependency_pkg_info(
            command_data,
            pkg_info,
            &dest_dir_path,
        )?;
    } else {
        install_non_local_dependency_pkg_info(
            tman_config,
            pkg_info,
            template_ctx,
            &dest_dir_path,
        )
        .await?;
    }

    Ok(())
}
