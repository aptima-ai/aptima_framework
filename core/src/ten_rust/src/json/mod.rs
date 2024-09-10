//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
mod bindings;

use anyhow::{Context, Result};
use json5;
use serde_json;

fn ten_remove_json_comments(json_with_comments: &str) -> Result<String> {
    let parsed_value: serde_json::Value =
        json5::from_str(json_with_comments)
            .context("Failed to parse the input as JSON5")?;

    let json_str = serde_json::to_string_pretty(&parsed_value)
        .context("Failed to serialize the parsed data into a JSON string")?;

    Ok(json_str)
}
