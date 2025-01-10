//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
mod bindings;

use std::collections::HashSet;
use std::path::Path;

use anyhow::{anyhow, Ok, Result};

fn resolve_interface_ref(
    ref_path: &str,
    base_dir: &str,
) -> Result<serde_json::Value> {
    if let Some(ref_file_path) = ref_path.strip_prefix("file://") {
        if base_dir.is_empty() {
            return Err(anyhow!("The base_dir can not be empty when resolving local file reference."));
        }

        let ref_file_full_path = Path::new(base_dir).join(ref_file_path);
        let ref_file = std::fs::File::open(ref_file_full_path)?;
        let reader = std::io::BufReader::new(ref_file);
        let ref_file_json: serde_json::Value = serde_json::from_reader(reader)?;

        if !ref_file_json.is_object() {
            return Err(anyhow::anyhow!(
                "The content of interface ref should be an object."
            ));
        }

        return Ok(ref_file_json.to_owned());
    }

    if ref_path.starts_with("https://") {
        return Err(anyhow!("Not supported yet."));
    }

    Err(anyhow!(
        "The ref path should start with 'file://' or 'https://'"
    ))
}

/// Resolves the interface content.
///
/// # Arguments
///
/// * `content` - The content of the interface, i.e., the value of the
///   `interface_in` or `interface_out` field in manifest.json. Ex:
///
///   <pre>
///    [
///      {
///        "name": "ia",
///        "cmd": [
///          {
///            "name": "foo"
///          }
///        ]
///      },
///      {
///        "name": "ib",
///        "$ref": "file://ib.json"
///      }
///    ]
///   </pre>
///
/// * `base_dir` - The base directory of the addon. If the content contains a
///   local file reference, the file will be resolved based on this directory.
pub fn ten_interface_schema_resolve_definition(
    content: &str,
    base_dir: &str,
) -> Result<serde_json::Value> {
    let mut content_json: serde_json::Value = serde_json::from_str(content)?;

    // Check if the interfaces contain duplicated names.
    let mut all_interface_names: HashSet<String> = HashSet::new();

    for item in content_json
        .as_array_mut()
        .ok_or(anyhow!("All interface should be an array."))?
    {
        let interface = item
            .as_object_mut()
            .ok_or(anyhow!("One interface should be an object."))?;

        let name = interface
            .get("name")
            .ok_or(anyhow!("Each interface should have a name."))?;

        let interface_name = name
            .as_str()
            .ok_or(anyhow!("The interface name should be a string."))?
            .to_string();

        let absent = all_interface_names.insert(interface_name);
        if !absent {
            return Err(anyhow::anyhow!(
                "The interface name should be unique."
            ));
        }

        if let Some(interface_ref) = interface.get("$ref") {
            let ref_path = interface_ref.as_str().ok_or(anyhow!(
                "The `$ref` field in interface should be a string."
            ))?;

            let mut ref_value = resolve_interface_ref(ref_path, base_dir)?;

            // Replace the reference with its content.
            interface.remove("$ref");
            if let Some(ref_obj) = ref_value.as_object_mut() {
                interface.append(ref_obj);
            } else {
                return Err(anyhow!("Expected object in resolved reference."));
            }
        }
    }

    Ok(content_json)
}
