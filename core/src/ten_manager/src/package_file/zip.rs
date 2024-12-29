//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
use std::{fs, io};
use std::{fs::File, path::Path};

use anyhow::Result;
use zip::write::SimpleFileOptions;
use zip::ZipWriter;

pub fn zip_files_to_file<P: AsRef<Path>>(
    files: Vec<P>,
    prefix: &Path,
    output_zip_file_path: &String,
) -> Result<()> {
    let output_file = File::create(output_zip_file_path)?;
    let mut output_zip = ZipWriter::new(output_file);

    // Use Deflated compression method for compatibility.
    let method = zip::CompressionMethod::Deflated;

    for path in files {
        let path = path.as_ref();
        let name = path.strip_prefix(prefix)?.to_string_lossy();

        let mut options =
            SimpleFileOptions::default().compression_method(method);

        #[cfg(unix)]
        {
            use std::os::unix::fs::PermissionsExt;

            // Fetch permissions
            let metadata = fs::metadata(path)?;
            let permissions = metadata.permissions();
            options = options.unix_permissions(permissions.mode());
        }

        if path.is_file() {
            output_zip.start_file(name, options)?;
            let mut f = File::open(path)?;
            io::copy(&mut f, &mut output_zip)?;
        } else if name != "." && name != "" {
            output_zip.add_directory(name, options)?;
        }
    }

    output_zip.finish()?;
    Ok(())
}
