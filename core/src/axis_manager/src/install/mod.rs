//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
pub mod installed_paths;
pub mod template;

use std::{
    collections::HashMap,
    fs::{self, OpenOptions},
    path::{Path, PathBuf},
    str::FromStr,
};

use anyhow::{anyhow, Context, Result};
use console::Emoji;
use tempfile::NamedTempFile;

use axis_rust::pkg_info::{
    constants::MANIFEST_JSON_FILENAME,
    find_to_be_replaced_local_pkgs, find_untracked_local_packages,
    get_pkg_info_from_path,
    manifest::dependency::ManifestDependency,
    pkg_basic_info::PkgBasicInfo,
    pkg_type::PkgType,
    pkg_type_and_name::PkgTypeAndName,
    supports::{is_pkg_supports_compatible_with, PkgSupport},
    PkgInfo,
};

use super::{config::TmanConfig, registry::get_package};
use crate::{
    cmd::cmd_install::{InstallCommand, LocalInstallMode},
    fs::{check_is_app_folder, copy_folder_recursively},
    log::tman_verbose_println,
    manifest_lock::{
        parse_manifest_lock_in_folder, write_pkg_lockfile, ManifestLock,
    },
    package_file::unzip::extract_and_process_zip,
    solver::solver_result::filter_solver_results_by_type_and_name,
};
use installed_paths::save_installed_paths;

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

    if Path::new(dest_dir_path).exists() {
        println!(
            "Destination directory '{}' already exists. Skipping copy/link.",
            dest_dir_path
        );
    } else {
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
                    std::os::windows::fs::symlink_dir(
                        src_dir_path,
                        &dest_dir_path,
                    )
                    .map_err(|e| {
                        anyhow::anyhow!(
                            "Failed to create directory symlink: {}",
                            e
                        )
                    })?;
                }
            }
        }
    }

    Ok(())
}

async fn install_non_local_dependency_pkg_info(
    tman_config: &TmanConfig,
    pkg_info: &PkgInfo,
    dest_dir_path: &String,
) -> Result<()> {
    let mut temp_file = NamedTempFile::new()?;
    get_package(tman_config, &pkg_info.url, &mut temp_file).await?;

    let mut installed_paths = extract_and_process_zip(
        &temp_file.path().to_string_lossy(),
        dest_dir_path,
        None,
    )?;

    // After installation (after decompression), check whether the content
    // of property.json is correct based on the decompressed
    // content.
    axis_rust::pkg_info::property::check_property_json_of_pkg(dest_dir_path)
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

    let target_path = PathBuf::from(&base_dir)
        .join(pkg_info.basic_info.type_and_name.name.clone());

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
            &dest_dir_path,
        )
        .await?;
    }

    Ok(())
}

pub fn is_package_installable_in_path(
    cwd: &Path,
    installing_pkg_type: &PkgType,
) -> Result<()> {
    match installing_pkg_type {
        PkgType::App => {
            return Err(anyhow!(
                "The package type 'app' is not allowed to be installed. \
                Use the 'create' command instead."
            ));
        }

        _ => {
            // All other package types must be installed into a APTIMA app folder.
            check_is_app_folder(cwd)?;
        }
    }

    Ok(())
}

pub fn update_package_manifest(
    base_pkg_info: &mut PkgInfo,
    added_dependency: &PkgInfo,
) -> Result<()> {
    if let Some(ref mut dependencies) =
        base_pkg_info.manifest.as_mut().unwrap().dependencies
    {
        let mut is_present = false;
        let mut deps_to_remove = Vec::new();

        for (i, dep) in dependencies.iter().enumerate() {
            match dep {
                ManifestDependency::RegistryDependency {
                    pkg_type,
                    name,
                    ..
                } => {
                    let manifest_dependency_type_and_name = PkgTypeAndName {
                        pkg_type: PkgType::from_str(pkg_type)?,
                        name: name.clone(),
                    };

                    if manifest_dependency_type_and_name
                        == added_dependency.basic_info.type_and_name
                    {
                        if !added_dependency.is_local_dependency {
                            is_present = true;
                        } else {
                            // The `manifest.json` specifies a registry
                            // dependency, but a local dependency is being
                            // added. Therefore, remove the original dependency
                            // item from `manifest.json`.
                            deps_to_remove.push(i);
                        }
                        break;
                    }
                }
                ManifestDependency::LocalDependency { path, .. } => {
                    let manifest_dependency_pkg_info =
                        match get_pkg_info_from_path(Path::new(&path), false) {
                            Ok(info) => info,
                            Err(_) => {
                                panic!(
                                    "Failed to get package info from path: {}",
                                    path
                                );
                            }
                        };

                    if manifest_dependency_pkg_info.basic_info.type_and_name
                        == added_dependency.basic_info.type_and_name
                    {
                        if added_dependency.is_local_dependency {
                            assert!(
                                added_dependency
                                    .local_dependency_path
                                    .is_some(),
                                "Should not happen."
                            );

                            if path
                                == added_dependency
                                    .local_dependency_path
                                    .as_ref()
                                    .unwrap()
                            {
                                is_present = true;
                            } else {
                                // The `manifest.json` specifies a local
                                // dependency, but a different local dependency
                                // is being added. Therefore, remove the
                                // original dependency item from
                                // `manifest.json`.
                                deps_to_remove.push(i);
                            }
                        } else {
                            // The `manifest.json` specifies a local dependency,
                            // but a registry dependency is being added.
                            // Therefore, remove the original dependency item
                            // from `manifest.json`.
                            deps_to_remove.push(i);
                        }
                        break;
                    }
                }
            }
        }

        // Remove dependencies to remove in reverse order to prevent index shift
        for &i in deps_to_remove.iter().rev() {
            dependencies.remove(i);
        }

        // If the added dependency does not exist in the `manifest.json`, add
        // it.
        if !is_present {
            dependencies.push(added_dependency.into());
        }
    } else {
        // If the `manifest.json` does not have a `dependencies` field, add the
        // dependency directly.
        base_pkg_info.manifest.clone().unwrap().dependencies =
            Some(vec![added_dependency.into()]);
    }

    let manifest_path: PathBuf =
        Path::new(&base_pkg_info.url).join(MANIFEST_JSON_FILENAME);
    let manifest_file = OpenOptions::new()
        .write(true)
        .truncate(true)
        .open(manifest_path)?;
    serde_json::to_writer_pretty(manifest_file, &base_pkg_info.manifest)?;

    Ok(())
}

pub fn write_pkgs_into_manifest_lock_file(
    pkgs: &Vec<&PkgInfo>,
    app_dir: &Path,
) -> Result<()> {
    // Check if manifest-lock.json exists.
    let old_manifest_lock = parse_manifest_lock_in_folder(app_dir);
    if old_manifest_lock.is_err() {
        println!("{}  Creating manifest-lock.json...", Emoji("🔒", ""));
    }

    let new_manifest_lock = ManifestLock::from(pkgs);

    let changed = write_pkg_lockfile(&new_manifest_lock, app_dir)?;

    // If the lock file is changed, print all changes.
    if changed && old_manifest_lock.is_ok() {
        println!("{}  Breaking manifest-lock.json...", Emoji("🔒", ""));

        new_manifest_lock.print_changes(&old_manifest_lock.ok().unwrap());
    }

    Ok(())
}

pub fn write_installing_pkg_into_manifest_file(
    pkg_info: &mut PkgInfo,
    solver_results: &[PkgInfo],
    pkg_type: &PkgType,
    pkg_name: &String,
) -> Result<()> {
    let suitable_pkgs = filter_solver_results_by_type_and_name(
        solver_results,
        Some(pkg_type),
        Some(pkg_name),
        true,
    )?;

    if suitable_pkgs.is_empty() {
        return Err(anyhow!(
            "Failed to find any of {}:{}.",
            pkg_type,
            pkg_name,
        ));
    }

    if suitable_pkgs.len() > 1 {
        return Err(anyhow!(
            "Found the possibility of multiple {}:{} being incorrect.",
            pkg_type,
            pkg_name
        ));
    }

    update_package_manifest(pkg_info, suitable_pkgs[0])?;

    Ok(())
}

/// Filter out the packages in `all_pkgs` that meet the current environment's
/// requirements and treat them as candidates.
pub fn filter_compatible_pkgs_to_candidates(
    tman_config: &TmanConfig,
    all_pkgs: &Vec<PkgInfo>,
    all_candidates: &mut HashMap<
        PkgTypeAndName,
        HashMap<PkgBasicInfo, PkgInfo>,
    >,
    support: &PkgSupport,
) {
    for existed_pkg in all_pkgs.to_owned().iter_mut() {
        tman_verbose_println!(
            tman_config,
            "Check support score for {:?}",
            existed_pkg
        );

        let compatible_score = is_pkg_supports_compatible_with(
            &existed_pkg.basic_info.supports,
            support,
        );

        if compatible_score >= 0 {
            existed_pkg.compatible_score = compatible_score;

            tman_verbose_println!(
                tman_config,
                "The existed {} package {} is compatible with the current system.",
                existed_pkg.basic_info.type_and_name.pkg_type,
                existed_pkg.basic_info.type_and_name.name
            );

            all_candidates
                .entry((&*existed_pkg).into())
                .or_default()
                .insert((&*existed_pkg).into(), existed_pkg.clone());
        } else {
            // The existed package is not compatible with the current system, so
            // it should not be considered as a candidate.
            tman_verbose_println!(
                tman_config,
                "The existed {} package {} is not compatible with the current \
                system.",
                existed_pkg.basic_info.type_and_name.pkg_type,
                existed_pkg.basic_info.type_and_name.name
            );
        }
    }
}

fn get_supports_str(pkg: &PkgInfo) -> String {
    let support_items: Vec<String> = pkg
        .basic_info
        .supports
        .iter()
        .filter_map(|s| match (s.os.as_ref(), s.arch.as_ref()) {
            (Some(os), Some(arch)) => {
                Some(format!("{:?}, {:?}", os, arch).to_lowercase())
            }
            (Some(os), None) => Some(format!("{:?}", os).to_lowercase()),
            (None, Some(arch)) => Some(format!("{:?}", arch).to_lowercase()),
            (None, None) => None,
        })
        .collect();

    if !support_items.is_empty() {
        format!(" ({})", support_items.join(", "))
    } else {
        String::new()
    }
}

pub fn compare_solver_results_with_existed_pkgs(
    solver_results: &[&PkgInfo],
    all_existing_local_pkgs: &[PkgInfo],
) -> bool {
    let local_pkgs = all_existing_local_pkgs.iter().collect::<Vec<&PkgInfo>>();

    let untracked_local_pkgs: Vec<&PkgInfo> =
        find_untracked_local_packages(solver_results, &local_pkgs);

    if !untracked_local_pkgs.is_empty() {
        println!(
            "{}  The following local packages do not appear in the dependency \
            tree:",
            Emoji("💡", "")
        );
        for pkg in untracked_local_pkgs {
            println!(
                " {}:{}@{}",
                pkg.basic_info.type_and_name.pkg_type,
                pkg.basic_info.type_and_name.name,
                pkg.basic_info.version
            );
        }
    }

    let to_be_replaced_local_pkgs: Vec<(&PkgInfo, &PkgInfo)> =
        find_to_be_replaced_local_pkgs(solver_results, &local_pkgs);

    let mut conflict = false;

    if !to_be_replaced_local_pkgs.is_empty() {
        conflict = true;

        println!(
            "{}  The following packages will be replaced:",
            Emoji("🔄", "")
        );
        for (new_pkg, old_pkg) in to_be_replaced_local_pkgs {
            let old_supports_str = get_supports_str(old_pkg);
            let new_supports_str = get_supports_str(new_pkg);

            if old_supports_str != new_supports_str {
                println!(
                    " {}:{}@{}{} -> {}:{}@{}{}",
                    old_pkg.basic_info.type_and_name.pkg_type,
                    old_pkg.basic_info.type_and_name.name,
                    old_pkg.basic_info.version,
                    old_supports_str,
                    new_pkg.basic_info.type_and_name.pkg_type,
                    new_pkg.basic_info.type_and_name.name,
                    new_pkg.basic_info.version,
                    new_supports_str
                );
            } else {
                println!(
                    " {}:{}@{} -> {}:{}@{}",
                    old_pkg.basic_info.type_and_name.pkg_type,
                    old_pkg.basic_info.type_and_name.name,
                    old_pkg.basic_info.version,
                    new_pkg.basic_info.type_and_name.pkg_type,
                    new_pkg.basic_info.type_and_name.name,
                    new_pkg.basic_info.version
                );
            }
        }
    }

    conflict
}
