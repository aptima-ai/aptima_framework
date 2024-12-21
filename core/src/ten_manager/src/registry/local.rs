//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
use std::fs::{self, File};
use std::io::Read;
use std::path::{Path, PathBuf};
use std::str::FromStr;

use anyhow::{anyhow, Context, Result};
use semver::Version;
use tempfile::NamedTempFile;
use walkdir::WalkDir;
use zip::ZipArchive;

use ten_rust::pkg_info::manifest::Manifest;
use ten_rust::pkg_info::pkg_type::PkgType;
use ten_rust::pkg_info::PkgInfo;

use super::{FoundResult, SearchCriteria};
use crate::config::TmanConfig;
use crate::constants::{MANIFEST_JSON_FILENAME, TEN_PACKAGE_FILE_EXTENSION};

pub async fn upload_package(
    base_url: &str,
    package_file_path: &str,
    pkg_info: &PkgInfo,
) -> Result<()> {
    let mut path_url = url::Url::parse(base_url)
        .map_err(|e| anyhow!("Invalid file URL: {}", e))?
        .to_file_path()
        .map_err(|_| anyhow!("Failed to convert file URL to path"))?
        .to_string_lossy()
        .into_owned();

    // Ensure the base URL ends with a '/'.
    path_url = if path_url.ends_with('/') {
        path_url.to_string()
    } else {
        format!("{}/", path_url)
    };

    // Construct the directory path.
    let dir_path = PathBuf::from(format!(
        "{}{}/{}/{}/",
        path_url,
        pkg_info.basic_info.type_and_name.pkg_type,
        pkg_info.basic_info.type_and_name.name,
        pkg_info.basic_info.version
    ));

    // Check if the directory exists, and only create it if it doesn't.
    if !dir_path.exists() {
        fs::create_dir_all(&dir_path).with_context(|| {
            format!("Failed to create directory '{}'", dir_path.display())
        })?;
    }

    // Construct the full file path for the new location.
    let file_stem = Path::new(package_file_path)
        .file_stem()
        .ok_or_else(|| anyhow::anyhow!("Invalid file path provided"))?
        .to_str()
        .ok_or_else(|| anyhow::anyhow!("Invalid unicode in file path"))?;

    let extension = Path::new(package_file_path)
        .extension()
        .and_then(|ext| ext.to_str())
        .map(|ext| format!(".{}", ext))
        .unwrap_or_default();

    let new_file_name = format!("{}_{}{}", file_stem, pkg_info.hash, extension);

    let full_path = dir_path.join(new_file_name);

    // Copy the file to the new path.
    fs::copy(package_file_path, &full_path).with_context(|| {
        format!(
            "Failed to copy file from '{}' to '{}'",
            package_file_path,
            full_path.display()
        )
    })?;

    Ok(())
}

pub async fn get_package(
    _tman_config: &TmanConfig,
    url: &str,
    temp_path: &mut NamedTempFile,
) -> Result<()> {
    let path_url_str = url::Url::parse(url)
        .map_err(|e| anyhow!("Invalid file URL: {}", e))?
        .to_file_path()
        .map_err(|_| anyhow!("Failed to convert file URL to path"))?
        .to_string_lossy()
        .into_owned();

    let path_url = PathBuf::from_str(&path_url_str)?;

    if !path_url.exists() {
        return Err(anyhow!(
            "The package directory does not exist: {}",
            path_url.display()
        ));
    }

    fs::copy(&path_url, temp_path.path())?;

    Ok(())
}

fn find_file_with_criteria(
    base_url: &Path,
    pkg_type: PkgType,
    name: &String,
    criteria: &SearchCriteria,
) -> Result<Vec<FoundResult>> {
    let target_path = base_url.join(pkg_type.to_string()).join(name);

    let mut results = Vec::<FoundResult>::new();

    for version_dir in WalkDir::new(target_path)
        .min_depth(1)
        .max_depth(1)
        .into_iter()
        .filter_map(|e| e.ok())
    {
        if criteria.version_req.matches(
            &Version::parse(
                version_dir.file_name().to_str().unwrap_or_default(),
            )
            .context("Invalid version format")?,
        ) {
            for entry in WalkDir::new(version_dir.path())
                .into_iter()
                .filter_map(|e| e.ok())
            {
                let path = entry.path();
                if path.extension().and_then(|s| s.to_str())
                    == Some(TEN_PACKAGE_FILE_EXTENSION)
                {
                    let file = File::open(path).with_context(|| {
                        format!("Failed to open {:?}", path)
                    })?;
                    let mut archive = ZipArchive::new(file)
                        .context("Failed to read zip archive")?;

                    for i in 0..archive.len() {
                        let mut file = archive.by_index(i).context(
                            "Failed to access file within zip archive",
                        )?;
                        if file.name() == MANIFEST_JSON_FILENAME {
                            let mut manifest_content = String::new();

                            file.read_to_string(&mut manifest_content)
                                .context("Failed to read manifest.json")?;

                            let manifest =
                                Manifest::from_str(&manifest_content)?;

                            results.push(FoundResult {
                                url: PathBuf::from(format!(
                                    "{}",
                                    url::Url::from_file_path(path).map_err(
                                        |_| anyhow!(
                                            "Failed to convert path to file URL"
                                        )
                                    )?,
                                )),
                                pkg_registry_info: (&manifest).try_into()?,
                            });

                            // Stop processing after finding the manifest.
                            break;
                        }
                    }
                }
            }
        }
    }

    Ok(results)
}

pub async fn get_package_list(
    _tman_config: &TmanConfig,
    base_url: &str,
    pkg_type: PkgType,
    name: &String,
    criteria: &SearchCriteria,
) -> Result<Vec<FoundResult>> {
    let mut path_url = url::Url::parse(base_url)
        .map_err(|e| anyhow!("Invalid file URL: {}", e))?
        .to_file_path()
        .map_err(|_| anyhow!("Failed to convert file URL to path"))?
        .to_string_lossy()
        .into_owned();

    // Ensure the base URL ends with a '/'.
    path_url = if path_url.ends_with('/') {
        path_url.to_string()
    } else {
        format!("{}/", path_url)
    };

    let result = find_file_with_criteria(
        Path::new(&path_url),
        pkg_type,
        name,
        criteria,
    )?;

    Ok(result)
}

pub async fn delete_package(
    base_url: &str,
    pkg_type: PkgType,
    name: &String,
    version: &Version,
    hash: &String,
) -> Result<()> {
    let mut path_url = url::Url::parse(base_url)
        .map_err(|e| anyhow!("Invalid file URL: {}", e))?
        .to_file_path()
        .map_err(|_| anyhow!("Failed to convert file URL to path"))?
        .to_string_lossy()
        .into_owned();

    // Ensure the base URL ends with a '/'.
    path_url = if path_url.ends_with('/') {
        path_url.to_string()
    } else {
        format!("{}/", path_url)
    };

    // Construct the directory path.
    let dir_path = PathBuf::from(format!(
        "{}{}/{}/{}/",
        path_url, pkg_type, name, version
    ));

    if dir_path.exists() {
        // Iterate over the files in the directory.
        for entry in fs::read_dir(&dir_path).with_context(|| {
            format!("Failed to read directory '{}'", dir_path.display())
        })? {
            let entry = entry.with_context(|| {
                format!(
                    "Failed to read entry in directory '{}'",
                    dir_path.display()
                )
            })?;
            let file_path = entry.path();

            // Check if the file name matches the pattern.
            if let Some(file_name) =
                file_path.file_name().and_then(|name| name.to_str())
            {
                if let Some(file_stem) = Path::new(file_name)
                    .file_stem()
                    .and_then(|stem| stem.to_str())
                {
                    if file_stem.ends_with(&format!("_{}", hash)) {
                        // Delete the file.
                        fs::remove_file(&file_path).with_context(|| {
                            format!(
                                "Failed to delete file '{}'",
                                file_path.display()
                            )
                        })?;
                    }
                }
            }
        }
    }

    Ok(())
}

#[cfg(test)]
mod tests {
    use semver::VersionReq;

    use super::*;

    #[test]
    fn test_semver_version_req() -> Result<()> {
        let mut rc =
            VersionReq::parse("3.0.0")?.matches(&Version::parse("3.0.0")?);
        assert!(rc);

        rc = VersionReq::parse("3.0.0")?.matches(&Version::parse("3.0.1")?);
        assert!(rc);

        rc = VersionReq::parse("3.0.0")?.matches(&Version::parse("3.1.0")?);
        assert!(rc);

        rc = VersionReq::parse("3.0.0")?
            .matches(&Version::parse("3.1.0-alpha")?);
        assert!(!rc);

        rc = VersionReq::parse("3.0.0")?
            .matches(&Version::parse("3.1.0+build3")?);
        assert!(rc);

        rc = VersionReq::parse("3.0.0")?
            .matches(&Version::parse("3.1.0-beta+build4")?);
        assert!(!rc);

        rc = VersionReq::parse("3.0.0")?.matches(&Version::parse("4.0.0")?);
        assert!(!rc);

        rc = VersionReq::parse("3.0.0")?.matches(&Version::parse("4.0.0")?);
        assert!(!rc);

        rc = VersionReq::parse(">=2.0.0")?.matches(&Version::parse("2.1.0+3")?);
        assert!(rc);

        rc = VersionReq::parse(">=2.0.0")?.matches(&Version::parse("2.0.0+3")?);
        assert!(rc);

        Ok(())
    }

    #[test]
    fn test_semver_version() -> Result<()> {
        let mut rc = Version::parse("3.0.1")? > Version::parse("3.0.0")?;
        assert!(rc);

        rc = Version::parse("3.0.0")? > Version::parse("3.0.0-alpha")?;
        assert!(rc);

        rc = Version::parse("3.0.0")? < Version::parse("3.0.1-alpha")?;
        assert!(rc);

        rc = Version::parse("3.0.0-alpha")? < Version::parse("3.0.0-beta")?;
        assert!(rc);

        rc = Version::parse("3.0.0")? == Version::parse("3.0.0")?;
        assert!(rc);

        Ok(())
    }
}
