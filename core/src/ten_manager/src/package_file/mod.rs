//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
pub mod unzip;
pub mod zip;

use std::path::Path;

use anyhow::Result;
use console::Emoji;
use globset::{GlobBuilder, GlobSetBuilder};
use ignore::{overrides::OverrideBuilder, WalkBuilder};
use zip::zip_files_to_file;

use ten_rust::pkg_info::manifest::parse_manifest_in_folder;
use ten_rust::pkg_info::PkgInfo;

use super::{config::TmanConfig, constants::TEN_PACKAGE_FILE_EXTENSION};
use crate::{
    constants::{DOT_TEN_DIR, PACKAGE_DIR_IN_DOT_TEN_DIR, TEN_PACKAGES_DIR},
    log::tman_verbose_println,
    utils::pathbuf_to_string_lossy,
};

pub fn get_package_zip_file_name(pkg_info: &PkgInfo) -> Result<String> {
    let output_zip_file_name = format!(
        "{}_{}.{}",
        pkg_info.basic_info.type_and_name.name,
        pkg_info.basic_info.version,
        TEN_PACKAGE_FILE_EXTENSION
    );

    Ok(output_zip_file_name)
}

pub fn create_package_zip_file(
    tman_config: &TmanConfig,
    generated_zip_file_name: &str,
    folder_to_zip: &Path,
) -> Result<String> {
    println!("{}  Creating package", Emoji("🚚", ":-)"));

    // Create the output directory.
    let output_dir = folder_to_zip
        .join(DOT_TEN_DIR)
        .join(PACKAGE_DIR_IN_DOT_TEN_DIR);
    if !output_dir.exists() {
        std::fs::create_dir_all(&output_dir)?;
    }

    let manifest = parse_manifest_in_folder(folder_to_zip)?;

    // Before packing the package, first check if the content of property.json
    // is correct.
    ten_rust::pkg_info::property::check_property_json_of_pkg(
        &folder_to_zip.to_string_lossy(),
    )
    .map_err(|e| {
        anyhow::anyhow!(
            "Failed to check property.json for {}:{}, {}",
            manifest.pkg_type,
            manifest.name,
            e
        )
    })?;

    // Generate the zip file.
    let output_zip_file_path = output_dir.join(generated_zip_file_name);
    if output_zip_file_path.exists() {
        std::fs::remove_file(&output_zip_file_path)?;
    }

    let output_zip_file_path_str =
        pathbuf_to_string_lossy(output_zip_file_path);

    // Collect files to include.
    let mut include_patterns: Option<Vec<String>> = None;
    if let Some(publish) = &manifest.package {
        if let Some(include) = &publish.include {
            include_patterns = Some(vec![]);

            include_patterns.as_mut().unwrap().extend(include.clone());
        }
    }

    let mut globset_builder = GlobSetBuilder::new();

    // manifest.json is needed for all TEN packages.
    globset_builder.add(
        GlobBuilder::new("manifest.json")
            .literal_separator(false)
            .build()?,
    );

    if include_patterns.as_ref().is_none() {
        // Include all folders and files by default.
        globset_builder
            .add(GlobBuilder::new("*").literal_separator(false).build()?);
    } else {
        for pattern in &include_patterns.unwrap() {
            globset_builder.add(
                GlobBuilder::new(pattern).literal_separator(true).build()?,
            );
        }
    }
    let include_globset = globset_builder.build()?;

    let mut ignore_builder = WalkBuilder::new(folder_to_zip);

    // Set the default values for the WalkBuilder.

    // Do not consider the file size.
    ignore_builder.max_filesize(None);

    // Do not consider the ignore files from the parent folders.
    ignore_builder.parents(false);

    // Ignore all the hidden files and folders.
    ignore_builder.hidden(true);

    ignore_builder.ignore(false);
    ignore_builder.git_ignore(false);

    let mut overrides = OverrideBuilder::new(folder_to_zip);

    // Add exclude pattern for DOT_TEN_DIR.
    overrides.add(&format!("!/{}/", DOT_TEN_DIR))?;

    // Add exclude pattern for TEN_PACKAGES_DIR.
    overrides.add(&format!("!/{}/", TEN_PACKAGES_DIR))?;

    let overrides = overrides.build()?;
    ignore_builder.overrides(overrides);

    let mut files_to_include = vec![];
    for result in ignore_builder.build() {
        match result {
            Ok(entry) => {
                let path = entry.path();
                let name = path.strip_prefix(folder_to_zip)?;

                if include_globset.is_match(name) {
                    files_to_include.push(path.to_path_buf());
                }
            }
            Err(err) => println!("Error: {:?}", err),
        }
    }

    tman_verbose_println!(tman_config, "Files to be packed:");
    for file in &files_to_include {
        tman_verbose_println!(tman_config, "> {:?}", file);
    }

    zip_files_to_file(
        files_to_include,
        folder_to_zip,
        &output_zip_file_path_str,
    )?;

    Ok(output_zip_file_path_str)
}
