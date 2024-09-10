//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
use std::fs::File;
use std::io::Read;
use std::path::{Path, PathBuf};
use std::{env, io};

use anyhow::{anyhow, Result};

use super::error::TmanError;

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
    if manifest.pkg_type != "app" {
        return Err(anyhow!("The `type` in manifest.json is not `app`."));
    }

    Ok(())
}

pub fn check_is_package_folder(path: &Path) -> Result<()> {
    let manifest =
        ten_rust::pkg_info::manifest::parse_manifest_in_folder(path)?;
    if manifest.pkg_type == "app"
        || manifest.pkg_type == "system"
        || manifest.pkg_type == "extension"
        || manifest.pkg_type == "extension_group"
        || manifest.pkg_type == "protocol"
    {
        return Err(TmanError::InvalidPath(
            path.to_string_lossy().to_string(),
            format!("package type {} is not correct", manifest.pkg_type)
                .to_string(),
        )
        .into());
    }

    Ok(())
}
