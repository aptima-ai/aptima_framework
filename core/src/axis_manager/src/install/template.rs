//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
use std::fs::{self, File};
use std::io::{self, Read, Write};
use std::path::Path;

use anyhow::{Context, Result};
use handlebars::Handlebars;
use zip::ZipArchive;

fn render_template(
    template: &str,
    template_ctx: &serde_json::Value,
) -> Result<String> {
    let reg = Handlebars::new();
    let rendered = reg
        .render_template(template, template_ctx)
        .with_context(|| "Failed to render template")?;
    Ok(rendered)
}

pub fn extract_and_process_zip_template_part(
    zip_path: &str,
    output_dir: &str,
    template_ctx: &serde_json::Value,
) -> Result<()> {
    // Open the ZIP file.
    let file = File::open(zip_path)?;
    let mut archive = ZipArchive::new(file)?;

    // Iterate through each entry in the ZIP.
    for i in 0..archive.len() {
        let mut file = archive.by_index(i)?;
        let is_template = file.name().ends_with(".tent");

        if is_template {
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
            }

            let out_path = Path::new(output_dir).join(file.mangled_name());

            let mut contents = String::new();
            let mut out_file = File::open(out_path.clone())?;
            out_file.read_to_string(&mut contents)?;
            let rendered = render_template(&contents, template_ctx)?;

            // Removes the .tent extension.
            let new_out_path = out_path.with_extension("");

            let mut new_out_file = File::create(new_out_path.clone())?;
            new_out_file.write_all(rendered.as_bytes())?;

            #[cfg(unix)]
            {
                // Set file permissions if it's a Unix system.
                use std::os::unix::fs::PermissionsExt;
                if let Some(mode) = file.unix_mode() {
                    fs::set_permissions(
                        &new_out_path,
                        fs::Permissions::from_mode(mode),
                    )?;
                }
            }

            // Remove the original file.
            fs::remove_file(out_path)?;
        }
    }

    Ok(())
}
