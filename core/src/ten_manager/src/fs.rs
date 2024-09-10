//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
use std::{
    collections::HashSet,
    fs,
    path::{Path, PathBuf},
};

use anyhow::Result;
use walkdir::WalkDir;

use remove_dir_all::remove_dir_all;

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
