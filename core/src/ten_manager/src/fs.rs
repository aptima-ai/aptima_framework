//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
use std::{
    collections::HashSet,
    fs,
    fs::File,
    io::Read,
    path::{Path, PathBuf},
};
use std::{env, io};

use anyhow::{anyhow, Result};
use fs_extra::dir::CopyOptions;
use walkdir::WalkDir;

use super::error::TmanError;
use remove_dir_all::remove_dir_all;
use ten_rust::pkg_info::pkg_type::PkgType;

fn merge_folders_internal(
    source_base: &Path,
    source: &Path,
    destination: &Path,
    inclusions: Option<&HashSet<PathBuf>>,
) -> Result<()> {
    for src_entry in WalkDir::new(source).into_iter().filter_map(|e| e.ok()) {
        let src_path = src_entry.path();
        let relative_src_path = src_path.strip_prefix(source).unwrap();
        let relative_src_path_on_base =
            src_path.strip_prefix(source_base).unwrap();

        // Check if current path is in the inclusions list.
        if inclusions.is_some() {
            let inclusions = inclusions.unwrap();
            if !inclusions.contains(relative_src_path_on_base) {
                continue;
            }
        }

        if src_path.exists() {
            let dest_path = destination.join(relative_src_path);

            if src_entry.file_type().is_dir() {
                if src_path == Path::new(source) {
                    continue;
                }

                if !dest_path.exists() {
                    fs::create_dir_all(&dest_path)?;
                }

                merge_folders_internal(
                    source_base,
                    src_path,
                    &dest_path,
                    None,
                )?;
                remove_dir_all(src_path)?;
            } else if src_entry.file_type().is_file() {
                if let Some(parent) = dest_path.parent() {
                    fs::create_dir_all(parent)?;
                }

                fs::copy(src_path, &dest_path)?;
                fs::remove_file(src_path)?;
            }
        }
    }

    Ok(())
}

pub fn merge_folders(
    source: &Path,
    destination: &Path,
    inclusions: &[String],
) -> Result<()> {
    // Adjust exclude paths for platform-specifics.
    let inclusions_set: HashSet<PathBuf> =
        inclusions.iter().map(PathBuf::from).collect();

    merge_folders_internal(source, source, destination, Some(&inclusions_set))
}

pub fn copy_folder_recursively(
    src_dir_path: &String,
    dest_dir_path: &String,
) -> Result<()> {
    let mut options = CopyOptions::new();

    // Copy the contents inside the directory.
    options.copy_inside = true;

    fs_extra::dir::copy(src_dir_path, dest_dir_path, &options)
        .map_err(|e| anyhow::anyhow!("Failed to copy directory: {}", e))?;

    Ok(())
}

pub fn get_cwd() -> Result<PathBuf> {
    // Attempt to get the current working directory.
    let cwd = match env::current_dir() {
        Ok(current_path) => current_path,
        // Convert the error to anyhow::Error and return.
        Err(e) => return Err(e.into()),
    };

    // If successful, return the current working directory path.
    Ok(cwd)
}

pub fn pathbuf_to_string(path_buf: PathBuf) -> Result<String> {
    if let Some(path_str) = path_buf.to_str() {
        return Ok(path_str.to_string());
    }

    Err(TmanError::InvalidPath(
        pathbuf_to_string_lossy(path_buf),
        "invalid characters".to_string(),
    )
    .into())
}

pub fn pathbuf_to_string_lossy(path_buf: PathBuf) -> String {
    // Convert the PathBuf to a String, replacing invalid UTF-8 sequences with �
    // (U+FFFD)
    path_buf.to_string_lossy().into_owned()
}

pub fn read_file_to_string<P: AsRef<Path>>(
    path: P,
) -> Result<String, TmanError> {
    let path_ref = path.as_ref();

    let mut file = File::open(path_ref).map_err(|e| match e.kind() {
        io::ErrorKind::NotFound => {
            TmanError::FileNotFound(path_ref.to_string_lossy().to_string())
        }
        _ => TmanError::InvalidPath(
            path_ref.to_string_lossy().to_string(),
            e.to_string(),
        ),
    })?;
    let mut contents = String::new();
    file.read_to_string(&mut contents)
        .map_err(|e| TmanError::ReadFileContentError(e.to_string()))?;
    Ok(contents)
}

pub fn check_is_app_folder(path: &Path) -> Result<()> {
    let manifest =
        ten_rust::pkg_info::manifest::parse_manifest_in_folder(path)?;
    if manifest.type_and_name.pkg_type != PkgType::App {
        return Err(anyhow!("The `type` in manifest.json is not `app`."));
    }

    Ok(())
}

pub fn check_is_package_folder(path: &Path) -> Result<()> {
    match ten_rust::pkg_info::manifest::parse_manifest_in_folder(path) {
        Ok(_) => Ok(()),
        Err(err) => Err(err),
    }
}

#[cfg(test)]
mod tests {
    // use super::*;
    // use crate::constants::PROPERTY_JSON_FILENAME;

    // #[test]
    // fn it_merge_files() -> Result<()> {
    //     let source = Path::new("/home/wei/MyData/Temp/tests/A");
    //     let destination = Path::new("/home/wei/MyData/Temp/tests/B");
    //     let inclusions = vec![
    //         "xxx/manifest.json".to_string(),
    //         PROPERTY_JSON_FILENAME.to_string(),
    //     ];

    //     merge_folders(source, destination, &inclusions)
    // }
}
