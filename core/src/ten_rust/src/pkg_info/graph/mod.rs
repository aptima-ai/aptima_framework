//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
mod check;

use std::{collections::HashMap, str::FromStr};

use anyhow::Result;
use serde::{Deserialize, Serialize};

use super::{pkg_type::PkgType, predefined_graphs::node::PkgNode, PkgInfo};
use crate::pkg_info::default_app_loc;

#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct Graph {
    // It's invalid that a graph does not contain any nodes.
    pub nodes: Vec<GraphNode>,

    #[serde(skip_serializing_if = "Option::is_none")]
    pub connections: Option<Vec<GraphConnection>>,
}

impl FromStr for Graph {
    type Err = anyhow::Error;

    fn from_str(s: &str) -> Result<Self, Self::Err> {
        let mut graph: Graph = serde_json::from_str(s)?;

        graph.validate_and_complete()?;

        // Return the parsed data.
        Ok(graph)
    }
}

impl Graph {
    pub fn validate_and_complete(&mut self) -> Result<()> {
        for node in &mut self.nodes {
            node.validate_and_complete()?;
        }

        for (node_idx, node) in self.nodes.iter().enumerate() {
            if node.app.is_empty() {
                return Err(anyhow::anyhow!(
                    "'app' field is missing in nodes[{}].",
                    node_idx
                ));
            }
        }

        Ok(())
    }

    pub fn check(
        &self,
        all_needed_pkgs: &HashMap<String, Vec<PkgInfo>>,
    ) -> Result<()> {
        self.check_if_nodes_duplicated()?;
        self.check_if_extensions_used_in_connections_have_defined_in_nodes()?;
        self.check_if_nodes_have_installed(all_needed_pkgs)?;
        self.check_connections_compatible(all_needed_pkgs)?;

        Ok(())
    }
}

#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct GraphNode {
    #[serde(rename = "type")]
    pub node_type: PkgType,
    pub name: String,
    pub addon: String,

    // extension group node does not contain extension_group field.
    #[serde(skip_serializing_if = "Option::is_none")]
    pub extension_group: Option<String>,

    // Default is 'localhost'.
    #[serde(default = "default_app_loc")]
    pub app: String,

    #[serde(skip_serializing_if = "Option::is_none")]
    pub property: Option<serde_json::Value>,
}

impl GraphNode {
    fn validate_and_complete(&mut self) -> Result<()> {
        // extension node must specify extension_group name.
        if self.node_type == PkgType::Extension
            && self.extension_group.is_none()
        {
            return Err(anyhow::anyhow!(
                "Node '{}' of type 'extension' must have an 'extension_group' defined.",
                self.name
            ));
        }

        Ok(())
    }
}

impl From<PkgNode> for GraphNode {
    fn from(pkg_node: PkgNode) -> Self {
        GraphNode {
            node_type: pkg_node.node_type.clone(),
            name: pkg_node.name.clone(),
            addon: pkg_node.addon.clone(),
            extension_group: pkg_node.extension_group.clone(),
            app: pkg_node.app.clone(),
            property: pkg_node.property.clone(),
        }
    }
}

#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct GraphConnection {
    // If a connection does not specify an app URI, it defaults to localhost.
    #[serde(default = "default_app_loc")]
    pub app: String,

    pub extension_group: String,
    pub extension: String,

    #[serde(skip_serializing_if = "Option::is_none")]
    pub cmd: Option<Vec<GraphMessageFlow>>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub data: Option<Vec<GraphMessageFlow>>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub audio_frame: Option<Vec<GraphMessageFlow>>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub video_frame: Option<Vec<GraphMessageFlow>>,
}

#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct GraphMessageFlow {
    pub name: String,
    pub dest: Vec<GraphDestination>,
}

#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct GraphDestination {
    // If a destination does not specify an app URI, it defaults to localhost.
    #[serde(default = "default_app_loc")]
    pub app: String,

    pub extension_group: String,
    pub extension: String,
}

#[cfg(test)]
mod tests {
    use std::str::FromStr;

    use crate::pkg_info::property::Property;

    use super::*;

    #[test]
    fn test_predefined_graph_has_no_extensions() {
        let property_str =
            include_str!("test_data_embed/predefined_graph_no_extensions.json");
        let mut property: Property = Property::from_str(property_str).unwrap();
        assert!(property.validate_and_complete().is_ok());

        let ten = property._ten.as_ref().unwrap();
        let predefined_graph =
            ten.predefined_graphs.as_ref().unwrap().first().unwrap();
        let graph = &predefined_graph.graph;
        let result = graph.check_if_nodes_duplicated();
        assert!(result.is_err());
        println!("Error: {:?}", result.err().unwrap());
    }

    #[test]
    fn test_predefined_graph_has_extension_duplicated() {
        let property_str = include_str!(
            "test_data_embed/predefined_graph_has_duplicated_extension.json"
        );
        let mut property: Property = Property::from_str(property_str).unwrap();
        assert!(property.validate_and_complete().is_ok());

        let ten = property._ten.as_ref().unwrap();
        let predefined_graph =
            ten.predefined_graphs.as_ref().unwrap().first().unwrap();
        let graph = &predefined_graph.graph;
        let result = graph.check_if_nodes_duplicated();
        assert!(result.is_err());
        println!("Error: {:?}", result.err().unwrap());
    }

    #[test]
    fn test_start_graph_cmd_has_extension_duplicated() {
        let cmd_str = include_str!(
            "test_data_embed/start_graph_cmd_has_duplicated_extension.json"
        );

        let mut graph: Graph = Graph::from_str(cmd_str).unwrap();
        assert!(graph.validate_and_complete().is_ok());

        let result = graph.check_if_nodes_duplicated();
        assert!(result.is_err());
        println!("Error: {:?}", result.err().unwrap());
    }

    #[test]
    fn test_predefined_graph_has_ext_group_duplicated() {
        let property_str = include_str!(
            "test_data_embed/predefined_graph_has_duplicated_ext_group.json"
        );
        let mut property: Property = Property::from_str(property_str).unwrap();
        assert!(property.validate_and_complete().is_ok());

        let ten = property._ten.as_ref().unwrap();
        let predefined_graph =
            ten.predefined_graphs.as_ref().unwrap().first().unwrap();

        let graph = &predefined_graph.graph;
        let result = graph.check_if_nodes_duplicated();
        assert!(result.is_err());
        println!("Error: {:?}", result.err().unwrap());
    }

    #[test]
    fn test_predefined_graph_has_unused_extension_group() {
        let property_str = include_str!(
            "test_data_embed/predefined_graph_unused_ext_group.json"
        );
        let mut property: Property = Property::from_str(property_str).unwrap();
        assert!(property.validate_and_complete().is_ok());

        let ten = property._ten.as_ref().unwrap();
        let predefined_graph =
            ten.predefined_graphs.as_ref().unwrap().first().unwrap();

        let graph = &predefined_graph.graph;
        let result = graph.check_if_nodes_duplicated();
        assert!(result.is_err());
        println!("Error: {:?}", result.err().unwrap());
    }

    #[test]
    fn test_predefined_graph_connection_src_not_found() {
        let property_str = include_str!(
            "test_data_embed/predefined_graph_connection_src_not_found.json"
        );
        let mut property: Property = Property::from_str(property_str).unwrap();
        assert!(property.validate_and_complete().is_ok());

        let ten = property._ten.as_ref().unwrap();
        let predefined_graph =
            ten.predefined_graphs.as_ref().unwrap().first().unwrap();

        let graph = &predefined_graph.graph;
        let result = graph
            .check_if_extensions_used_in_connections_have_defined_in_nodes();
        assert!(result.is_err());
        println!("Error: {:?}", result.err().unwrap());
    }

    #[test]
    fn test_predefined_graph_connection_dest_not_found() {
        let property_str = include_str!(
            "test_data_embed/predefined_graph_connection_dest_not_found.json"
        );
        let mut property: Property = Property::from_str(property_str).unwrap();
        assert!(property.validate_and_complete().is_ok());

        let ten = property._ten.as_ref().unwrap();
        let predefined_graph =
            ten.predefined_graphs.as_ref().unwrap().first().unwrap();

        let graph = &predefined_graph.graph;
        let result = graph
            .check_if_extensions_used_in_connections_have_defined_in_nodes();
        assert!(result.is_err());
        println!("Error: {:?}", result.err().unwrap());
    }
}
