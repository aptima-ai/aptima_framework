//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
use std::{
    collections::{HashMap, HashSet},
    env,
    fs::{self, OpenOptions},
    path::{Path, PathBuf},
    str::FromStr,
    time::Instant,
};

use anyhow::{Ok, Result};
use clap::{Arg, ArgAction, ArgMatches, Command};
use console::Emoji;
use indicatif::HumanDuration;
use inquire::Confirm;
use semver::VersionReq;

use crate::{
    config::TmanConfig,
    constants::{APP_DIR_IN_DOT_TEN_DIR, DOT_TEN_DIR, MANIFEST_JSON_FILENAME},
    dep_and_candidate::get_all_candidates_from_deps,
    error::TmanError,
    install::PkgIdentityMapping,
    log::tman_verbose_println,
    manifest_lock::{
        parse_manifest_lock_in_folder, write_pkg_lockfile, ManifestLock,
    },
    package_info::tman_get_all_existed_pkgs_info_of_app,
    solver::{
        solve::solve_all,
        solver_result::{
            extract_solver_results_from_raw_solver_results,
            filter_solver_results_by_type_and_name,
            install_solver_results_in_app_folder,
            install_solver_results_in_standalone_mode,
        },
    },
    utils::{check_is_app_folder, check_is_package_folder},
};
use ten_rust::pkg_info::manifest::{
    dependency::ManifestDependency, parse_manifest_in_folder,
};
use ten_rust::pkg_info::{
    dependencies::{DependencyRelationship, PkgDependency},
    find_to_be_replaced_local_pkgs, find_untracked_local_packages,
    get_pkg_info_from_path,
    pkg_identity::PkgIdentity,
    pkg_type::PkgType,
    supports::{is_pkg_supports_compatible_with, Arch, Os, PkgSupport},
    PkgInfo,
};

#[derive(Debug)]
pub struct InstallCommand {
    pub package_type: Option<String>,
    pub package_name: Option<String>,
    pub support: PkgSupport,
    pub template_mode: bool,
    pub template_data: HashMap<String, String>,
}

pub fn create_sub_cmd(args_cfg: &crate::cmd_line::ArgsCfg) -> Command {
    Command::new("install")
        .about("Install a package. For more detailed usage, run 'install -h'")
        .arg(
            Arg::new("PACKAGE_TYPE")
                .help("The type of the package")
                .value_parser(args_cfg.pkg_type.possible_values.clone())
                .required(false)
                // If PACKAGE_TYPE is provided, PACKAGE_NAME must be too.
                .requires("PACKAGE_NAME"),
        )
        .arg(
            Arg::new("PACKAGE_NAME")
                .help("The name of the package")
                .required(false),
        )
        .arg(
            Arg::new("OS")
                .long("os")
                .help("The operating system")
                .value_parser(args_cfg.os.possible_values.clone())
                .required(false),
        )
        .arg(
            Arg::new("ARCH")
                .long("arch")
                .help("The CPU architecture")
                .value_parser(args_cfg.arch.possible_values.clone())
                .required(false),
        )
        .arg(
            Arg::new("BUILD_TYPE")
                .long("build-type")
                .help("The build type")
                .value_parser(args_cfg.build_type.possible_values.clone())
                .required(false),
        )
        .arg(
            Arg::new("TEMPLATE_MODE")
                .long("template-mode")
                .action(ArgAction::SetTrue)
                .requires("PACKAGE_TYPE")
                .requires("PACKAGE_NAME")
                .help("If provided, 'template-mode' is true; otherwise, it's false"),
        )
        .arg(
            Arg::new("TEMPLATE_DATA")
                .long("template-data")
                .value_name("KEY=VALUE")
                .action(ArgAction::Append)
                .requires("TEMPLATE_MODE")
                .help("Sets a key-value pair, e.g., --template-data key=value")
        )
}

pub fn parse_sub_cmd(sub_cmd_args: &ArgMatches) -> InstallCommand {
    let mut cmd = InstallCommand {
        package_type: sub_cmd_args.get_one::<String>("PACKAGE_TYPE").cloned(),
        package_name: sub_cmd_args.get_one::<String>("PACKAGE_NAME").cloned(),
        support: PkgSupport {
            os: sub_cmd_args
                .get_one::<String>("OS")
                .and_then(|s| s.parse::<Os>().ok()),
            arch: sub_cmd_args
                .get_one::<String>("ARCH")
                .and_then(|s| s.parse::<Arch>().ok()),
        },
        template_mode: *sub_cmd_args
            .get_one::<bool>("TEMPLATE_MODE")
            .unwrap_or(&false),
        template_data: HashMap::new(),
    };

    let _ = cmd.support.set_defaults();

    if let Some(kv_pairs) = sub_cmd_args.get_many::<String>("TEMPLATE_DATA") {
        for pair in kv_pairs {
            let mut split = pair.splitn(2, '=');
            if let (Some(key), Some(value)) = (split.next(), split.next()) {
                cmd.template_data.insert(key.to_string(), value.to_string());
            }
        }
    }

    cmd
}

fn is_package_installable_in_cwd(
    cwd: &Path,
    desired_pkg_type: &PkgType,
) -> Result<()> {
    match desired_pkg_type {
        PkgType::App => {
            // The app must not be installed into a TEN package folder.
            if check_is_package_folder(cwd).is_ok() {
                return Err(TmanError::Custom(
                    "There is already a TEN package in the current folder. The TEN APP must be installed in a separate folder."
                        .to_string(),
                )
                .into());
            }
        }

        PkgType::Extension => {
            let manifest_path = cwd.join(MANIFEST_JSON_FILENAME);
            if !manifest_path.exists() {
                // An extension can be independently installed in a non-TEN
                // directory. This is mainly to allow developers to easily
                // develop, compile, test, and release an extension.
                return Ok(());
            }

            // Otherwise, the extension must be installed in a TEN app folder.
            check_is_app_folder(cwd)?;
        }

        _ => {
            // All other package types must be installed into a TEN app folder.
            check_is_app_folder(cwd)?;
        }
    }

    Ok(())
}

// The so-called "standalone" package installation means directly installing the
// package into the current working directory without considering the directory
// structure of the TEN app.
fn is_installing_package_standalone(
    cwd: &Path,
    desired_pkg_type: &PkgType,
) -> Result<bool> {
    match desired_pkg_type {
        PkgType::App | PkgType::Extension => {
            let manifest_path = cwd.join(MANIFEST_JSON_FILENAME);
            if !manifest_path.exists() {
                // An extension can be independently installed in a non-TEN
                // directory. This is mainly to allow developers to easily
                // develop, compile, test, and release an extension.
                return Ok(true);
            }

            if parse_manifest_in_folder(cwd).is_ok() {
                return Ok(false);
            }

            Ok(true)
        }
        // Currently, other standalone installation methods are not supported.
        _ => Ok(false),
    }
}

fn update_package_manifest(
    base_pkg_info: &mut PkgInfo,
    added_pkg_info: &PkgInfo,
) -> Result<()> {
    if let Some(ref mut dependencies) =
        base_pkg_info.manifest.as_mut().unwrap().dependencies
    {
        let is_present = dependencies.iter().any(|dep| {
            dep.pkg_type == added_pkg_info.pkg_identity.pkg_type.to_string()
                && dep.name == added_pkg_info.pkg_identity.name
        });

        if !is_present {
            dependencies.push(ManifestDependency {
                pkg_type: added_pkg_info.pkg_identity.pkg_type.to_string(),
                name: added_pkg_info.pkg_identity.name.clone(),
                version: added_pkg_info.version.to_string(),
            });
        }
    } else {
        base_pkg_info.manifest.clone().unwrap().dependencies =
            Some(vec![ManifestDependency {
                pkg_type: added_pkg_info.pkg_identity.pkg_type.to_string(),
                name: added_pkg_info.pkg_identity.name.clone(),
                version: added_pkg_info.version.to_string(),
            }]);
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

fn write_pkgs_into_lock(pkgs: &Vec<&PkgInfo>, app_dir: &Path) -> Result<()> {
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

fn parse_pkg_name_version(
    pkg_name_version: &str,
) -> Result<(String, VersionReq)> {
    let parts: Vec<&str> = pkg_name_version.split('@').collect();
    if parts.len() == 2 {
        Ok((parts[0].to_string(), VersionReq::parse(parts[1])?))
    } else {
        Ok((pkg_name_version.to_string(), VersionReq::STAR))
    }
}

fn filter_compatible_pkgs_to_candidates(
    tman_config: &TmanConfig,
    all_existing_local_pkgs: &Vec<PkgInfo>,
    all_candidates: &mut HashMap<PkgIdentity, HashSet<PkgInfo>>,
    support: &PkgSupport,
) {
    for existed_pkg in all_existing_local_pkgs.to_owned().iter_mut() {
        let compatible_score =
            is_pkg_supports_compatible_with(&existed_pkg.supports, support);

        if compatible_score >= 0 {
            existed_pkg.compatible_score = compatible_score;

            all_candidates
                .entry(existed_pkg.pkg_identity.clone())
                .or_default()
                .insert(existed_pkg.clone());
        } else {
            // The existed package is not compatible with the current system, so
            // it should not be considered as a candidate.
            tman_verbose_println!(
                tman_config,
                "The existed {} package {} is not compatible \
with the current system.",
                existed_pkg.pkg_identity.pkg_type,
                existed_pkg.pkg_identity.name
            );
        }
    }
}

fn get_supports_str(pkg: &PkgInfo) -> String {
    let support_items: Vec<String> = pkg
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

fn compare_solver_results_with_existed_pkgs(
    solver_results: &[&PkgInfo],
    all_existing_local_pkgs: &[PkgInfo],
) -> bool {
    let local_pkgs = all_existing_local_pkgs.iter().collect::<Vec<&PkgInfo>>();

    let untracked_local_pkgs: Vec<&PkgInfo> =
        find_untracked_local_packages(solver_results, &local_pkgs);

    if !untracked_local_pkgs.is_empty() {
        println!(
            "{}  The following local packages do not \
appear in the dependency tree:",
            Emoji("💡", "")
        );
        for pkg in untracked_local_pkgs {
            println!(
                " {}:{}@{}",
                pkg.pkg_identity.pkg_type, pkg.pkg_identity.name, pkg.version
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
                    old_pkg.pkg_identity.pkg_type,
                    old_pkg.pkg_identity.name,
                    old_pkg.version,
                    old_supports_str,
                    new_pkg.pkg_identity.pkg_type,
                    new_pkg.pkg_identity.name,
                    new_pkg.version,
                    new_supports_str
                );
            } else {
                println!(
                    " {}:{}@{} -> {}:{}@{}",
                    old_pkg.pkg_identity.pkg_type,
                    old_pkg.pkg_identity.name,
                    old_pkg.version,
                    new_pkg.pkg_identity.pkg_type,
                    new_pkg.pkg_identity.name,
                    new_pkg.version
                );
            }
        }
    }

    conflict
}

pub async fn execute_cmd(
    tman_config: &TmanConfig,
    command_data: InstallCommand,
) -> Result<()> {
    tman_verbose_println!(tman_config, "Executing install command");
    tman_verbose_println!(tman_config, "{:?}", command_data);
    tman_verbose_println!(tman_config, "{:?}", tman_config);

    let started = Instant::now();

    let cwd = crate::utils::get_cwd()?;

    // The package affected by tman install command, which is the root declared
    // in the resolver.
    //
    // 1. If installing a package in standalone mode, the affected package is
    //    the installed package itself, ex: app or extension.
    //
    // 2. Otherwise, the affected package is always the app, as the desired
    //    package must be installed in a TEN app in this case.
    let affected_pkg_name;
    let mut affected_pkg_type = PkgType::App;

    let mut is_standalone_installing = false;
    let mut preinstall_chdir_path: Option<PathBuf> = None;

    // If `tman install` is run within the scope of an app, then the app and
    // those addons (extensions, extension_groups, ...) installed in the app
    // directory are all considered initial_input_pkgs.
    let mut initial_input_pkgs = vec![];
    let mut all_candidates: HashMap<PkgIdentity, HashSet<PkgInfo>> =
        HashMap::new();

    // 'all_existing_local_pkgs' contains all the packages which are already
    // located in the app directory.
    //
    // After the completed dependency tree is resolved,
    // 'all_existing_local_pkgs' will be compared with the solver results:
    //
    // *) If some of these packages are not included in the dependency tree,
    // then users will be prompted that these packages can be added to the
    // dependencies.
    //
    // *) If some of these packages are not compatible with packages in
    // the dependency tree, then users will be questioned whether to overwrite
    // them with the new packages or quit the installation.
    //
    // 'all_existing_local_pkgs' is only used in non-standalone mode and is not
    // used for standalone installation (execute 'tman install' in an extension
    // folder).
    let mut all_existing_local_pkgs: Vec<PkgInfo> = vec![];

    // The initial value of extra_dependencies is the package specified to be
    // installed via the command line (if any).
    let mut extra_dependencies = vec![];

    // `extra_dependency_relationships` contain TEN packages, and each TEN
    // package is the main entity depended upon by its corresponding
    // extra_dependencies."
    let mut extra_dependency_relationships = vec![];

    // If template mode is used, the final name of the installed package is a
    // new name. Therefore, pkg_identity_mappings represent the mapping between
    // the source package name and the destination package name.
    let mut pkg_identity_mappings = vec![];

    // Get the specified template data.
    let mut template_ctx = None;
    let template_data = serde_json::to_value(&command_data.template_data)?;

    // The locked_pkgs comes from a lock file in the app folder.
    let mut locked_pkgs: Option<HashMap<PkgIdentity, PkgInfo>> = None;

    if command_data.template_mode {
        template_ctx = Some(&template_data);
    }

    let mut app_pkg: Option<PkgInfo> = None;
    let mut desired_pkg_type: Option<PkgType> = None;
    let mut desired_pkg_src_name: Option<String> = None;

    let desired_pkg_dest_name = template_ctx
        .and_then(|ctx| ctx.get("package_name").and_then(|val| val.as_str()));

    if let Some(package_type_str) = command_data.package_type {
        // Case 1: tman install <package_type> <package_name>

        let desired_pkg_type_: PkgType = package_type_str.parse()?;
        let (desired_pkg_src_name_, desired_pkg_src_version_) =
            parse_pkg_name_version(&command_data.package_name.unwrap())?;

        desired_pkg_type = Some(desired_pkg_type_.clone());
        desired_pkg_src_name = Some(desired_pkg_src_name_.clone());

        // First, check that the package we want to install can be installed
        // within the current directory structure.
        is_package_installable_in_cwd(&cwd, &desired_pkg_type_)?;

        is_standalone_installing =
            is_installing_package_standalone(&cwd, &desired_pkg_type_)?;
        if is_standalone_installing {
            affected_pkg_type = desired_pkg_type_.clone();
            affected_pkg_name = desired_pkg_src_name_.clone();

            if let Some(desired_pkg_dest_name) = desired_pkg_dest_name {
                preinstall_chdir_path =
                    Some(PathBuf::from_str(desired_pkg_dest_name)?);
            } else {
                preinstall_chdir_path =
                    Some(PathBuf::from_str(&desired_pkg_src_name_)?);
            }
        } else {
            // If it is not a standalone install, then the `cwd` must be within
            // the base directory of a TEN app.
            let app_pkg_ = get_pkg_info_from_path(&cwd)?;
            affected_pkg_name = app_pkg_.pkg_identity.name.clone();

            // Push the app itself into the initial_input_pkgs.
            initial_input_pkgs.push(get_pkg_info_from_path(&cwd)?);

            all_existing_local_pkgs =
                tman_get_all_existed_pkgs_info_of_app(tman_config, &cwd)?;

            // Add existing packages into all_candidates only if the compatible
            // score of the package is >= 0.
            filter_compatible_pkgs_to_candidates(
                tman_config,
                &all_existing_local_pkgs,
                &mut all_candidates,
                &command_data.support,
            );

            let extra_dependency_relationship = DependencyRelationship {
                pkg_identity: app_pkg_.pkg_identity.clone(),
                version: app_pkg_.version.clone(),
                dependency: PkgDependency {
                    pkg_identity: PkgIdentity {
                        pkg_type: desired_pkg_type_.clone(),
                        name: desired_pkg_src_name_.clone(),
                    },
                    version_req: desired_pkg_src_version_.clone(),
                },
            };
            extra_dependency_relationships.push(extra_dependency_relationship);

            app_pkg = Some(app_pkg_);
        }

        let dep = PkgDependency::new(
            desired_pkg_type_.clone(),
            desired_pkg_src_name_.clone(),
            desired_pkg_src_version_,
        );
        extra_dependencies.push(dep);

        if let Some(desired_pkg_dest_name) = desired_pkg_dest_name {
            let pkg_identity_mapping = PkgIdentityMapping {
                src_pkg_identity: PkgIdentity {
                    pkg_type: desired_pkg_type_.clone(),
                    name: desired_pkg_src_name_,
                },
                dest_pkg_identity: PkgIdentity {
                    pkg_type: desired_pkg_type_,
                    name: desired_pkg_dest_name.to_string(),
                },
            };
            pkg_identity_mappings.push(pkg_identity_mapping);
        }
    } else {
        // Case 2: tman install

        let manifest = parse_manifest_in_folder(&cwd)?;
        affected_pkg_type = manifest.pkg_type.parse::<PkgType>()?;
        affected_pkg_name = manifest.name.clone();

        match affected_pkg_type {
            PkgType::App => {
                check_is_app_folder(&cwd)?;

                // Push the app itself into the initial_input_pkgs.
                initial_input_pkgs.push(get_pkg_info_from_path(&cwd)?);

                all_existing_local_pkgs =
                    tman_get_all_existed_pkgs_info_of_app(tman_config, &cwd)?;
                filter_compatible_pkgs_to_candidates(
                    tman_config,
                    &all_existing_local_pkgs,
                    &mut all_candidates,
                    &command_data.support,
                );
            }

            PkgType::Extension => {
                // Install all dependencies of the extension package, but a APP
                // folder should be created first.
                preinstall_chdir_path =
                    Some(Path::new(DOT_TEN_DIR).join(APP_DIR_IN_DOT_TEN_DIR));

                if let Some(ref path) = preinstall_chdir_path {
                    fs::create_dir_all(path)?;
                }

                initial_input_pkgs.push(get_pkg_info_from_path(&cwd)?);

                filter_compatible_pkgs_to_candidates(
                    tman_config,
                    &initial_input_pkgs,
                    &mut all_candidates,
                    &command_data.support,
                );
            }

            _ => {
                return Err(TmanError::Custom(
                    "Current folder should be a TEN APP or Extension package."
                        .to_string(),
                )
                .into());
            }
        }
    }

    // Get the locked pkgs from the lock file in the app folder.
    if !is_standalone_installing {
        let mut app_dir = cwd.clone();
        if let Some(preinstall_chdir_path) = &preinstall_chdir_path {
            app_dir = cwd.join(preinstall_chdir_path);
        }

        let manifest_lock = parse_manifest_lock_in_folder(&app_dir);

        if manifest_lock.is_ok() {
            locked_pkgs = Some(manifest_lock.unwrap().get_pkgs());
        }
    }

    // Get all possible candidates according to the input packages and extra
    // dependencies.
    let all_candidates = get_all_candidates_from_deps(
        tman_config,
        &command_data.support,
        initial_input_pkgs,
        all_candidates,
        &extra_dependencies,
        locked_pkgs.as_ref(),
    )
    .await?;

    println!("{}  Resolving packages...", Emoji("🔍", ""),);

    // Find the answer that satisfies all dependencies.
    let results = solve_all(
        tman_config,
        &affected_pkg_name,
        &affected_pkg_type,
        &extra_dependency_relationships,
        &all_candidates,
        locked_pkgs.as_ref(),
    )?;

    // Print out the answer.
    tman_verbose_println!(tman_config, "Result:");
    for result in &results {
        tman_verbose_println!(tman_config, " {:?}", result);
    }
    tman_verbose_println!(tman_config, "");

    // Get the information of the resultant packages.
    let solver_results = extract_solver_results_from_raw_solver_results(
        &results,
        &all_candidates,
    )?;

    if is_standalone_installing {
        // If installing a standalone package (ex: app or extension), the
        // package itself must be installed first.

        let affected_pkg = filter_solver_results_by_type_and_name(
            &solver_results,
            Some(&affected_pkg_type),
            Some(&affected_pkg_name),
            true,
        )?;

        if affected_pkg.is_empty() {
            return Err(TmanError::Custom(format!(
                "Failed to find any of {}:{}.",
                affected_pkg_type, affected_pkg_name,
            ))
            .into());
        }
        if affected_pkg.len() > 1 {
            return Err(TmanError::Custom(format!(
                "Found the possibility of multiple {}:{} being incorrect.",
                affected_pkg_type, affected_pkg_name
            ))
            .into());
        }

        // TODO(Liu): Do some check after the installation. Ex: the package is
        // installed in the template mode, however the package might not have a
        // manifest.json template file to rerender the package name. In this
        // case, the installed package is invalid, because as a spec of TEN
        // package, the installed folder name should be the same as the package
        // name. Anyway, the package name in manifest.json should be forced to
        // replace with the `package-name` arg?
        install_solver_results_in_standalone_mode(
            tman_config,
            &affected_pkg,
            &pkg_identity_mappings,
            template_ctx,
            &cwd,
        )
        .await?;
    }

    // If we need to switch to a specific folder before installing other
    // packages, do so now.
    let mut app_dir = cwd.clone();
    if let Some(preinstall_chdir_path) = &preinstall_chdir_path {
        env::set_current_dir(cwd.join(preinstall_chdir_path))?;
        app_dir = cwd.join(preinstall_chdir_path);
    }

    // We have the following three cases:
    //
    // Case 1: install a standalone app, i.e.: tman install app ...
    // Case 2: install a standalone extension, i.e.: tman install extension ...
    // Case 3: install dependencies of a package, i.e.: run `tman install` in a
    // package folder, which is either a TEN app or extension.
    //
    // In case 2, after the package has been installed, its dependencies can NOT
    // be installed automatically, as the installation directory of
    // dependencies is different in this case.
    if affected_pkg_type == PkgType::App || !is_standalone_installing {
        // Install all the dependencies which the app depends on.
        let remaining_solver_results = filter_solver_results_by_type_and_name(
            &solver_results,
            Some(&affected_pkg_type),
            Some(&affected_pkg_name),
            false,
        )?;

        // Compare the remaining_solver_results with the all_existing_local_pkgs
        // to check if there are any local packages that need to be deleted or
        // replaced.
        let has_conflict = compare_solver_results_with_existed_pkgs(
            &remaining_solver_results,
            &all_existing_local_pkgs,
        );

        if has_conflict && !tman_config.assume_yes {
            // "y" for continuing to install, "n" for stopping.
            let ans = Confirm::new(
                "Warning!!! Some local packages will be overwritten, \
do you want to continue?",
            )
            .with_default(false)
            .prompt();

            match ans {
                std::result::Result::Ok(true) => {
                    // continue to install
                }
                std::result::Result::Ok(false) => {
                    // stop
                    return Ok(());
                }
                Err(_) => {
                    // stop
                    return Ok(());
                }
            }
        }

        write_pkgs_into_lock(&remaining_solver_results, &app_dir)?;

        install_solver_results_in_app_folder(
            tman_config,
            &remaining_solver_results,
            &pkg_identity_mappings,
            template_ctx,
            &app_dir,
        )
        .await?;
    }

    // Change back to the original folder.
    if preinstall_chdir_path.is_some() {
        env::set_current_dir(cwd)?;
    }

    // Write package info to manifest.json.
    if let Some(mut app_pkg) = app_pkg {
        if !command_data.template_mode
            && desired_pkg_type.is_some()
            && desired_pkg_src_name.is_some()
        {
            let desired_pkg = filter_solver_results_by_type_and_name(
                &solver_results,
                desired_pkg_type.as_ref(),
                desired_pkg_src_name.as_ref(),
                true,
            )?;

            if desired_pkg.is_empty() {
                return Err(TmanError::Custom(format!(
                    "Failed to find any of {}:{}.",
                    desired_pkg_type.unwrap(),
                    desired_pkg_src_name.unwrap(),
                ))
                .into());
            }
            if desired_pkg.len() > 1 {
                return Err(TmanError::Custom(format!(
                    "Found the possibility of multiple {}:{} being incorrect.",
                    desired_pkg_type.unwrap(),
                    desired_pkg_src_name.unwrap()
                ))
                .into());
            }

            update_package_manifest(&mut app_pkg, desired_pkg[0])?;
        }
    }

    println!(
        "{}  Install successfully in {}",
        Emoji("🏆", ":-)"),
        HumanDuration(started.elapsed())
    );

    Ok(())
}
