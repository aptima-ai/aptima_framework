//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
pub mod predefined_graph;

use std::{
    collections::HashMap,
    fs,
    path::{Path, PathBuf},
    str::FromStr,
};

use anyhow::{Context, Result};
use serde::{Deserialize, Serialize};
use serde_json::Value;

use super::{
    constants::{PROPERTY_JSON_FILENAME, TEN_FIELD_IN_PROPERTY},
    utils::read_file_to_string,
};
use crate::pkg_info::graph::is_app_default_loc_or_none;
use crate::{json_schema, pkg_info::localhost};
use predefined_graph::PredefinedGraph;

#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct Property {
    #[serde(skip_serializing_if = "Option::is_none")]
    pub _ten: Option<TenInProperty>,

    #[serde(flatten)]
    pub additional_fields: HashMap<String, Value>,
}

impl FromStr for Property {
    type Err = anyhow::Error;

    fn from_str(s: &str) -> Result<Self, Self::Err> {
        let mut property: Property = serde_json::from_str(s)?;
        property.validate_and_complete()?;
        Ok(property)
    }
}

impl Property {
    pub fn validate_and_complete(&mut self) -> Result<()> {
        if let Some(_ten) = &mut self._ten {
            _ten.validate_and_complete()?;
        }
        Ok(())
    }

    pub fn dump_property_to_file(
        &self,
        property_file_path: &PathBuf,
    ) -> Result<()> {
        let mut property_json = serde_json::Map::new();

        if let Some(_ten) = &self._ten {
            property_json.insert(
                TEN_FIELD_IN_PROPERTY.to_string(),
                serde_json::to_value(_ten)?,
            );
        }

        // Merge additional fields back into the property JSON
        for (key, value) in &self.additional_fields {
            property_json.insert(key.clone(), value.clone());
        }

        let new_property_str = serde_json::to_string_pretty(&property_json)?;
        fs::write(property_file_path, new_property_str)?;

        Ok(())
    }

    pub fn get_app_uri(&self) -> String {
        if let Some(_ten) = &self._ten {
            if let Some(uri) = &_ten.uri {
                return uri.clone();
            }
        }

        localhost()
    }
}

#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct TenInProperty {
    #[serde(skip_serializing_if = "Option::is_none")]
    pub predefined_graphs: Option<Vec<PredefinedGraph>>,

    #[serde(skip_serializing_if = "is_app_default_loc_or_none")]
    pub uri: Option<String>,

    #[serde(flatten)]
    pub additional_fields: HashMap<String, Value>,
}

impl TenInProperty {
    pub fn validate_and_complete(&mut self) -> Result<()> {
        if let Some(graphs) = &mut self.predefined_graphs {
            {
                let mut seen_graph_names = std::collections::HashSet::new();
                for graph in graphs.iter() {
                    if !seen_graph_names.insert(&graph.name) {
                        return Err(anyhow::anyhow!(
                            "Duplicate predefined graph name detected: '{}'. \
                            Each predefined_graph must have a unique 'name'.",
                            graph.name
                        ));
                    }
                }
            }

            for graph in graphs {
                graph.validate_and_complete()?;
            }
        }

        if self.uri.is_none() {
            self.uri = Some(localhost());
        }

        Ok(())
    }
}

pub fn check_property_json_of_pkg(pkg_dir: &str) -> Result<()> {
    let property_json_path = Path::new(pkg_dir).join(PROPERTY_JSON_FILENAME);
    if !property_json_path.exists() {
        return Ok(());
    }

    json_schema::ten_validate_property_json_file(
        property_json_path.to_str().unwrap(),
    )
}

pub fn parse_property_from_file<P: AsRef<Path>>(
    property_file_path: P,
) -> Result<Property> {
    // Read the contents of the property.json file.
    let content = read_file_to_string(property_file_path)?;

    Property::from_str(&content)
}

pub fn parse_property_in_folder(
    folder_path: &Path,
) -> Result<Option<Property>> {
    // Path to the property.json file.
    let property_path = folder_path.join(PROPERTY_JSON_FILENAME);
    if !property_path.exists() {
        return Ok(None);
    }

    // Read and parse the property.json file.
    let property =
        parse_property_from_file(&property_path).with_context(|| {
            format!("Failed to load {}.", property_path.display())
        })?;

    // Validate the property schema only if it is present.
    json_schema::ten_validate_property_json_file(&property_path).with_context(
        || format!("Failed to validate {}.", property_path.display()),
    )?;

    Ok(Some(property))
}

#[cfg(test)]
mod tests {
    use crate::pkg_info::graph::msg_conversion::{
        MsgConversionMode, MsgConversionType,
    };

    use super::*;
    use std::fs::{self};
    use tempfile::tempdir;

    #[test]
    fn test_parse_property_with_known_fields() {
        let json_data = r#"
        {
            "_ten": {
                "predefined_graphs": [],
                "uri": "http://example.com"
            }
        }
        "#;

        let property: Property = json_data.parse().unwrap();

        assert!(property._ten.is_some());
        let ten_in_property = property._ten.unwrap();

        assert_eq!(ten_in_property.uri.unwrap(), "http://example.com");
        assert!(ten_in_property.predefined_graphs.is_some());
        assert!(ten_in_property.additional_fields.is_empty());
        assert!(property.additional_fields.is_empty());
    }

    #[test]
    fn test_parse_property_with_additional_fields() {
        let json_data = r#"
        {
            "_ten": {
                "predefined_graphs": [],
                "uri": "http://example.com",
                "extra_field_1": "value1"
            },
            "global_field_1": "global_value1"
        }
        "#;

        let property: Property = json_data.parse().unwrap();

        assert!(property._ten.is_some());
        let ten_in_property = property._ten.unwrap();
        assert_eq!(ten_in_property.uri.unwrap(), "http://example.com");
        assert!(ten_in_property.predefined_graphs.is_some());
        assert_eq!(
            ten_in_property
                .additional_fields
                .get("extra_field_1")
                .unwrap(),
            "value1"
        );

        assert_eq!(
            property.additional_fields.get("global_field_1").unwrap(),
            "global_value1"
        );
    }

    #[test]
    fn test_dump_property_to_file() {
        let dir = tempdir().unwrap();
        let file_path = dir.path().join("property.json");

        let ten_in_property = TenInProperty {
            predefined_graphs: Some(vec![]),
            uri: Some(String::from("http://example.com")),
            additional_fields: HashMap::new(),
        };

        let property = Property {
            _ten: Some(ten_in_property),
            additional_fields: HashMap::new(),
        };

        property.dump_property_to_file(&file_path).unwrap();

        let saved_content = fs::read_to_string(file_path).unwrap();
        let expected_content = r#"
        {
            "_ten": {
                "predefined_graphs": [],
                "uri": "http://example.com"
            }
        }
        "#
        .trim();

        let saved_json: serde_json::Value =
            serde_json::from_str(&saved_content).unwrap();
        let expected_json: serde_json::Value =
            serde_json::from_str(expected_content).unwrap();

        assert_eq!(saved_json, expected_json);
    }

    #[test]
    fn test_parse_and_dump_property_with_additional_fields() {
        let json_data = r#"
        {
            "_ten": {
                "predefined_graphs": [],
                "uri": "http://example.com",
                "extra_field_1": "value1"
            },
            "global_field_1": "global_value1"
        }
        "#;

        let property: Property = json_data.parse().unwrap();

        let dir = tempdir().unwrap();
        let file_path = dir.path().join("property.json");

        property.dump_property_to_file(&file_path).unwrap();

        let saved_content = fs::read_to_string(file_path).unwrap();
        let saved_json: serde_json::Value =
            serde_json::from_str(&saved_content).unwrap();
        let original_json: serde_json::Value =
            serde_json::from_str(json_data).unwrap();

        assert_eq!(saved_json, original_json);
    }

    #[test]
    fn test_dump_property_without_localhost_app_in_graph() {
        let json_data = include_str!("test_data_embed/property.json");
        let property: Property = json_data.parse().unwrap();
        assert!(property._ten.is_some());
        let ten = property._ten.as_ref().unwrap();
        let predefined_graphs = ten.predefined_graphs.as_ref().unwrap();
        let nodes = &predefined_graphs.first().as_ref().unwrap().graph.nodes;
        let node = nodes.first().unwrap();
        assert_eq!(node.get_app_uri(), localhost());

        let dir = tempdir().unwrap();
        let file_path = dir.path().join("property.json");
        property.dump_property_to_file(&file_path).unwrap();

        let saved_content = fs::read_to_string(file_path).unwrap();
        eprintln!("{}", saved_content);
        assert_eq!(saved_content.find(localhost().as_str()), None);
    }

    #[test]
    fn test_dump_property_with_msg_conversion() {
        let prop_str = include_str!(
            "test_data_embed/dump_property_with_msg_conversion.json"
        );
        let property: Property = prop_str.parse().unwrap();
        assert!(property._ten.is_some());

        let ten = property._ten.as_ref().unwrap();
        let predefined_graphs = ten.predefined_graphs.as_ref().unwrap();
        let connections = &predefined_graphs
            .first()
            .as_ref()
            .unwrap()
            .graph
            .connections
            .as_ref()
            .unwrap();
        let connection = connections.first().unwrap();
        let cmd = connection.cmd.as_ref().unwrap();
        assert_eq!(cmd.len(), 1);

        let cmd_dest = &cmd.first().unwrap().dest;
        assert_eq!(cmd_dest.len(), 1);

        let msg_conversion =
            cmd_dest.first().unwrap().msg_conversion.as_ref().unwrap();
        assert_eq!(
            msg_conversion.msg.conversion_type,
            MsgConversionType::PerProperty
        );

        let rules = &msg_conversion.msg.rules;
        assert!(rules.keep_original.is_none());
        assert_eq!(rules.rules.len(), 2);

        let rule = rules.rules.first().unwrap();
        assert_eq!(rule.conversion_mode, MsgConversionMode::FixedValue);
        assert!(rule.original_path.is_none());
        assert_eq!(rule.value.as_ref().unwrap(), "hello_mapping");

        let dir = tempdir().unwrap();
        let file_path = dir.path().join("property.json");

        property.dump_property_to_file(&file_path).unwrap();

        let saved_content = fs::read_to_string(file_path).unwrap();
        eprintln!("{}", saved_content);
        assert!(saved_content.contains("msg_conversion"));
    }
}
