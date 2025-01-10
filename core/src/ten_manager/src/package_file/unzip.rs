//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
use std::fs::{self, File};
use std::io::{self};
use std::path::Path;

use anyhow::Result;
use zip::ZipArchive;

use crate::install::installed_paths::{sort_installed_paths, InstalledPaths};
use crate::install::template::extract_and_process_zip_template_part;

fn extract_and_process_zip_normal_part(
    zip_path: &str,
    output_dir: &str,
) -> Result<InstalledPaths> {
    // Open the ZIP file.
    let file = File::open(zip_path)?;
    let mut archive = ZipArchive::new(file)?;

    let mut installed_paths = InstalledPaths { paths: Vec::new() };

    // Iterate through each entry in the ZIP.
    for i in 0..archive.len() {
        let mut file = archive.by_index(i)?;
        let is_template = file.name().ends_with(".tent");
        if is_template {
            continue;
        }

        let out_path = Path::new(output_dir).join(file.mangled_name());

        // Check if the entry is a file or directory.
        if file.name().ends_with('/') {
            fs::create_dir_all(&out_path)?;
        } else {
            // Ensure that the parent directory exists.
            if let Some(p) = out_path.parent() {
                if !p.exists() {
                    fs::create_dir_all(p)?;
                }
            }

            let mut out_file = File::create(&out_path)?;
            io::copy(&mut file, &mut out_file)?;

            #[cfg(unix)]
            {
                // Set file permissions if it's a Unix system.
                use std::os::unix::fs::PermissionsExt;

                // Set file permissions if it's a Unix system.
                if let Some(mode) = file.unix_mode() {
                    fs::set_permissions(
                        &out_path,
                        fs::Permissions::from_mode(mode),
                    )?;
                }
            }
        }

        let relative_path = out_path
            .strip_prefix(output_dir)?
            .to_str()
            .unwrap()
            .to_string();
        installed_paths.paths.push(relative_path);
    }

    Ok(installed_paths)
}

pub fn extract_and_process_zip(
    zip_path: &str,
    output_dir: &str,
    template_ctx: Option<&serde_json::Value>,
) -> Result<InstalledPaths> {
    let mut installed_paths =
        extract_and_process_zip_normal_part(zip_path, output_dir)?;

    if let Some(template_ctx) = template_ctx {
        // In the 'template' mode.
        extract_and_process_zip_template_part(
            zip_path,
            output_dir,
            template_ctx,
        )?;

        let rtet_paths: Vec<_> = installed_paths
            .paths
            .iter()
            .filter(|p| p.ends_with(".tent"))
            .cloned()
            .collect();

        for rtet_path in rtet_paths {
            installed_paths.paths.retain(|p| p != &rtet_path);

            let original_path = rtet_path.trim_end_matches(".tent").to_string();
            if !installed_paths.paths.contains(&original_path) {
                installed_paths.paths.push(original_path);
            }
        }
    }

    // Custom sort: directories after their contents.
    sort_installed_paths(&mut installed_paths);

    Ok(installed_paths)
}
