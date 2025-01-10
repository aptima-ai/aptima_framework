//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
mod check;
mod constants;
pub mod msg_conversion;

use std::{collections::HashMap, str::FromStr};

use anyhow::Result;
use msg_conversion::MsgAndResultConversion;
use serde::{Deserialize, Serialize};

use super::{pkg_type::PkgType, pkg_type_and_name::PkgTypeAndName, PkgInfo};
use crate::pkg_info::localhost;

/// The state of the 'app' field declaration in all nodes in the graph.
///
/// There might be the following cases for the 'app' field declaration:
///
/// - Case 1: neither of the nodes has declared the 'app' field. The state will
///   be `NoneDeclared`.
///
/// - Case 2: all nodes have declared the 'app' field, and all of them have the
///   same value. Ex:
///
///     {
///         "nodes": [
///             {
///                 "type": "extension",
///                 "app": "http://localhost:8000",
///                 "addon": "addon_1",
///                 "name": "ext_1",
///                 "extension_group": "some_group"
///             },
///             {
///                 "type": "extension",
///                 "app": "http://localhost:8000",
///                 "addon": "addon_2",
///                 "name": "ext_2",
///                 "extension_group": "another_group"
///             }
///         ]
///     }
///
///   The state will be `UniformDeclared`.
///
/// - Case 3: all nodes have declared the 'app' field, but they have different
///   values.
///
///     {
///         "nodes": [
///             {
///                 "type": "extension",
///                 "app": "http://localhost:8000",
///                 "addon": "addon_1",
///                 "name": "ext_1",
///                 "extension_group": "some_group"
///             },
///             {
///                 "type": "extension",
///                 "app": "msgpack://localhost:8001",
///                 "addon": "addon_2",
///                 "name": "ext_2",
///                 "extension_group": "another_group"
///             }
///         ]
///     }
///
///   The state will be `MixedDeclared`.
///
/// - Case 4: some nodes have declared the 'app' field, and some have not. It's
///   illegal.
///
/// In the view of the 'app' field declaration, there are two types of graphs:
///
/// * Single-app graph: the state is `NoneDeclared` or `UniformDeclared`.
/// * Multi-app graph: the state is `MixedDeclared`.

#[derive(Debug, Clone, PartialEq)]
pub enum GraphNodeAppDeclaration {
    NoneDeclared,
    UniformDeclared,
    MixedDeclared,
}

impl GraphNodeAppDeclaration {
    pub fn is_single_app_graph(&self) -> bool {
        match self {
            GraphNodeAppDeclaration::NoneDeclared => true,
            GraphNodeAppDeclaration::UniformDeclared => true,
            GraphNodeAppDeclaration::MixedDeclared => false,
        }
    }
}

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
    fn determine_graph_node_app_declaration(
        &self,
    ) -> Result<GraphNodeAppDeclaration> {
        let mut nodes_have_declared_app = 0;
        let mut app_uris = std::collections::HashSet::new();

        for (idx, node) in self.nodes.iter().enumerate() {
            if let Some(app_uri) = &node.app {
                if app_uri.is_empty() {
                    return Err(anyhow::anyhow!(
                        "nodes[{}]: {}",
                        idx,
                        constants::ERR_MSG_APP_IS_EMPTY
                    ));
                }

                app_uris.insert(app_uri);
                nodes_have_declared_app += 1;
            }
        }

        if nodes_have_declared_app != 0
            && nodes_have_declared_app != self.nodes.len()
        {
            return Err(anyhow::anyhow!(
                constants::ERR_MSG_APP_DECLARATION_MISMATCH
            ));
        }

        match app_uris.len() {
            0 => Ok(GraphNodeAppDeclaration::NoneDeclared),
            1 => Ok(GraphNodeAppDeclaration::UniformDeclared),
            _ => Ok(GraphNodeAppDeclaration::MixedDeclared),
        }
    }

    pub fn validate_and_complete(&mut self) -> Result<()> {
        let graph_node_app_declaration =
            self.determine_graph_node_app_declaration()?;

        for (idx, node) in &mut self.nodes.iter_mut().enumerate() {
            node.validate_and_complete(&graph_node_app_declaration)
                .map_err(|e| anyhow::anyhow!("nodes[{}]: {}", idx, e))?;
        }

        if let Some(connections) = &mut self.connections {
            for (idx, connection) in connections.iter_mut().enumerate() {
                connection
                    .validate_and_complete(&graph_node_app_declaration)
                    .map_err(|e| {
                        anyhow::anyhow!("connections[{}].{}", idx, e)
                    })?;
            }
        }

        Ok(())
    }

    pub fn check(
        &self,
        existed_pkgs_of_all_apps: &HashMap<String, Vec<PkgInfo>>,
    ) -> Result<()> {
        self.check_if_nodes_duplicated()?;
        self.check_if_extensions_used_in_connections_have_defined_in_nodes()?;
        self.check_if_nodes_have_installed(existed_pkgs_of_all_apps, false)?;
        self.check_connections_compatible(existed_pkgs_of_all_apps, false)?;
        self.check_extension_uniqueness_in_connections()?;
        self.check_message_names()?;

        Ok(())
    }

    pub fn check_for_single_app(
        &self,
        existed_pkgs_of_app: &HashMap<String, Vec<PkgInfo>>,
    ) -> Result<()> {
        assert!(existed_pkgs_of_app.len() == 1);

        self.check_if_nodes_duplicated()?;
        self.check_if_extensions_used_in_connections_have_defined_in_nodes()?;
        self.check_if_nodes_have_installed(existed_pkgs_of_app, true)?;

        // In a single app, there is no information about pkg_info of other
        // apps, neither the message schemas.
        self.check_connections_compatible(existed_pkgs_of_app, true)?;
        self.check_extension_uniqueness_in_connections()?;
        self.check_message_names()?;

        Ok(())
    }
}

#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct GraphNode {
    #[serde(flatten)]
    pub type_and_name: PkgTypeAndName,

    pub addon: String,

    // extension group node does not contain extension_group field.
    #[serde(skip_serializing_if = "Option::is_none")]
    pub extension_group: Option<String>,

    #[serde(skip_serializing_if = "is_app_default_loc_or_none")]
    pub app: Option<String>,

    #[serde(skip_serializing_if = "Option::is_none")]
    pub property: Option<serde_json::Value>,
}

impl GraphNode {
    fn validate_and_complete(
        &mut self,
        graph_node_app_declaration: &GraphNodeAppDeclaration,
    ) -> Result<()> {
        // Extension node must specify extension_group name.
        if self.type_and_name.pkg_type == PkgType::Extension
            && self.extension_group.is_none()
        {
            return Err(anyhow::anyhow!(
                "Node '{}' of type 'extension' must have an 'extension_group' defined.",
                self.type_and_name.name
            ));
        }

        if let Some(app) = &self.app {
            if app.as_str() == localhost() {
                let err_msg =
                    if graph_node_app_declaration.is_single_app_graph() {
                        constants::ERR_MSG_APP_LOCALHOST_DISALLOWED_SINGLE
                    } else {
                        constants::ERR_MSG_APP_LOCALHOST_DISALLOWED_MULTI
                    };

                return Err(anyhow::anyhow!(err_msg));
            }
        } else {
            self.app = Some(localhost().to_string());
        }

        Ok(())
    }

    pub fn get_app_uri(&self) -> &str {
        // The 'app' should be assigned after 'validate_and_complete' is called,
        // so it should not be None.
        self.app.as_ref().unwrap().as_str()
    }
}

#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct GraphConnection {
    #[serde(skip_serializing_if = "is_app_default_loc_or_none")]
    pub app: Option<String>,

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

impl GraphConnection {
    fn validate_and_complete(
        &mut self,
        graph_node_app_declaration: &GraphNodeAppDeclaration,
    ) -> Result<()> {
        if let Some(app) = &self.app {
            if app.as_str() == localhost() {
                let err_msg =
                    if graph_node_app_declaration.is_single_app_graph() {
                        constants::ERR_MSG_APP_LOCALHOST_DISALLOWED_SINGLE
                    } else {
                        constants::ERR_MSG_APP_LOCALHOST_DISALLOWED_MULTI
                    };

                return Err(anyhow::anyhow!(err_msg));
            }

            if *graph_node_app_declaration
                == GraphNodeAppDeclaration::NoneDeclared
            {
                return Err(anyhow::anyhow!(
                    constants::ERR_MSG_APP_SHOULD_NOT_DECLARED
                ));
            }
        } else {
            if *graph_node_app_declaration
                != GraphNodeAppDeclaration::NoneDeclared
            {
                return Err(anyhow::anyhow!(
                    constants::ERR_MSG_APP_SHOULD_DECLARED
                ));
            }

            self.app = Some(localhost().to_string());
        }

        if let Some(cmd) = &mut self.cmd {
            for (idx, cmd_flow) in cmd.iter_mut().enumerate() {
                cmd_flow
                    .validate_and_complete(graph_node_app_declaration)
                    .map_err(|e| anyhow::anyhow!("cmd[{}].{}", idx, e))?;
            }
        }

        if let Some(data) = &mut self.data {
            for (idx, data_flow) in data.iter_mut().enumerate() {
                data_flow
                    .validate_and_complete(graph_node_app_declaration)
                    .map_err(|e| anyhow::anyhow!("data[{}].{}", idx, e))?;
            }
        }

        if let Some(audio_frame) = &mut self.audio_frame {
            for (idx, audio_flow) in audio_frame.iter_mut().enumerate() {
                audio_flow
                    .validate_and_complete(graph_node_app_declaration)
                    .map_err(|e| {
                        anyhow::anyhow!("audio_frame[{}].{}", idx, e)
                    })?;
            }
        }

        if let Some(video_frame) = &mut self.video_frame {
            for (idx, video_flow) in video_frame.iter_mut().enumerate() {
                video_flow
                    .validate_and_complete(graph_node_app_declaration)
                    .map_err(|e| {
                        anyhow::anyhow!("video_frame[{}].{}", idx, e)
                    })?;
            }
        }

        Ok(())
    }

    pub fn get_app_uri(&self) -> &str {
        self.app.as_ref().unwrap().as_str()
    }
}

#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct GraphMessageFlow {
    pub name: String,
    pub dest: Vec<GraphDestination>,
}

impl GraphMessageFlow {
    fn validate_and_complete(
        &mut self,
        graph_node_app_declaration: &GraphNodeAppDeclaration,
    ) -> Result<()> {
        for (idx, dest) in &mut self.dest.iter_mut().enumerate() {
            dest.validate_and_complete(graph_node_app_declaration)
                .map_err(|e| anyhow::anyhow!("dest[{}]: {}", idx, e))?;
        }

        Ok(())
    }
}

#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct GraphDestination {
    #[serde(skip_serializing_if = "is_app_default_loc_or_none")]
    pub app: Option<String>,

    pub extension: String,

    #[serde(skip_serializing_if = "Option::is_none")]
    pub msg_conversion: Option<MsgAndResultConversion>,
}

impl GraphDestination {
    fn validate_and_complete(
        &mut self,
        graph_node_app_declaration: &GraphNodeAppDeclaration,
    ) -> Result<()> {
        if let Some(app) = &self.app {
            if app.as_str() == localhost() {
                let err_msg =
                    if graph_node_app_declaration.is_single_app_graph() {
                        constants::ERR_MSG_APP_LOCALHOST_DISALLOWED_SINGLE
                    } else {
                        constants::ERR_MSG_APP_LOCALHOST_DISALLOWED_MULTI
                    };

                return Err(anyhow::anyhow!(err_msg));
            }

            if *graph_node_app_declaration
                == GraphNodeAppDeclaration::NoneDeclared
            {
                return Err(anyhow::anyhow!(
                    constants::ERR_MSG_APP_SHOULD_NOT_DECLARED
                ));
            }
        } else {
            if *graph_node_app_declaration
                != GraphNodeAppDeclaration::NoneDeclared
            {
                return Err(anyhow::anyhow!(
                    constants::ERR_MSG_APP_SHOULD_DECLARED
                ));
            }

            self.app = Some(localhost().to_string());
        }

        if let Some(msg_conversion) = &self.msg_conversion {
            msg_conversion.validate().map_err(|e| {
                anyhow::anyhow!("invalid msg_conversion: {}", e)
            })?;
        }

        Ok(())
    }

    pub fn get_app_uri(&self) -> &str {
        self.app.as_ref().unwrap().as_str()
    }
}

pub fn is_app_default_loc_or_none(app: &Option<String>) -> bool {
    app.is_none() || app.as_ref().unwrap().as_str() == localhost()
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
        let property: Property = Property::from_str(property_str).unwrap();
        let aptima = property._ten.as_ref().unwrap();
        let predefined_graph =
            aptima.predefined_graphs.as_ref().unwrap().first().unwrap();
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
        let property: Property = Property::from_str(property_str).unwrap();
        let aptima = property._ten.as_ref().unwrap();
        let predefined_graph =
            aptima.predefined_graphs.as_ref().unwrap().first().unwrap();
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

        let graph: Graph = Graph::from_str(cmd_str).unwrap();
        let result = graph.check_if_nodes_duplicated();
        assert!(result.is_err());
        println!("Error: {:?}", result.err().unwrap());
    }

    #[test]
    fn test_predefined_graph_connection_src_not_found() {
        let property_str = include_str!(
            "test_data_embed/predefined_graph_connection_src_not_found.json"
        );
        let property: Property = Property::from_str(property_str).unwrap();
        let aptima = property._ten.as_ref().unwrap();
        let predefined_graph =
            aptima.predefined_graphs.as_ref().unwrap().first().unwrap();

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
        let property: Property = Property::from_str(property_str).unwrap();
        let aptima = property._ten.as_ref().unwrap();
        let predefined_graph =
            aptima.predefined_graphs.as_ref().unwrap().first().unwrap();

        let graph = &predefined_graph.graph;
        let result = graph
            .check_if_extensions_used_in_connections_have_defined_in_nodes();
        assert!(result.is_err());
        println!("Error: {:?}", result.err().unwrap());
    }

    #[test]
    fn test_predefined_graph_node_app_localhost() {
        let property_str = include_str!(
            "test_data_embed/predefined_graph_connection_app_localhost.json"
        );
        let property = Property::from_str(property_str);

        // 'localhost' is not allowed in graph definition.
        assert!(property.is_err());
        println!("Error: {:?}", property);

        let msg = property.err().unwrap().to_string();
        assert!(
            msg.contains(constants::ERR_MSG_APP_LOCALHOST_DISALLOWED_SINGLE)
        );
    }

    #[test]
    fn test_start_graph_cmd_single_app_node_app_localhost() {
        let graph_str = include_str!(
            "test_data_embed/start_graph_cmd_single_app_node_app_localhost.json"
        );
        let graph = Graph::from_str(graph_str);

        // 'localhost' is not allowed in graph definition.
        assert!(graph.is_err());
        println!("Error: {:?}", graph);

        let msg = graph.err().unwrap().to_string();
        assert!(
            msg.contains(constants::ERR_MSG_APP_LOCALHOST_DISALLOWED_SINGLE)
        );
    }

    #[test]
    fn test_start_graph_cmd_multi_apps_node_app_localhost() {
        let graph_str = include_str!(
            "test_data_embed/start_graph_cmd_multi_apps_node_app_localhost.json"
        );
        let graph = Graph::from_str(graph_str);

        // 'localhost' is not allowed in graph definition.
        assert!(graph.is_err());
        println!("Error: {:?}", graph);

        let msg = graph.err().unwrap().to_string();
        assert!(msg.contains(constants::ERR_MSG_APP_LOCALHOST_DISALLOWED_MULTI));
    }

    #[test]
    fn test_predefined_graph_connection_app_localhost() {
        let property_str = include_str!(
            "test_data_embed/predefined_graph_connection_app_localhost.json"
        );
        let property = Property::from_str(property_str);

        // 'localhost' is not allowed in graph definition.
        assert!(property.is_err());
        println!("Error: {:?}", property);

        let msg = property.err().unwrap().to_string();
        assert!(
            msg.contains(constants::ERR_MSG_APP_LOCALHOST_DISALLOWED_SINGLE)
        );
    }

    #[test]
    fn test_predefined_graph_app_in_nodes_not_all_declared() {
        let property_str = include_str!(
            "test_data_embed/predefined_graph_app_in_nodes_not_all_declared.json"
        );
        let property = Property::from_str(property_str);

        // Either all nodes should have 'app' declared, or none should, but not
        // a mix of both.
        assert!(property.is_err());
        println!("Error: {:?}", property);

        let msg = property.err().unwrap().to_string();
        assert!(msg.contains(
            "Either all nodes should have 'app' declared, or none should, but not a mix of both."
        ));
    }

    #[test]
    fn test_predefined_graph_app_in_connections_not_all_declared() {
        let property_str = include_str!(
            "test_data_embed/predefined_graph_app_in_connections_not_all_declared.json"
        );
        let property = Property::from_str(property_str);

        // The 'app' can not be none, as it has been declared in nodes.
        assert!(property.is_err());
        println!("Error: {:?}", property);

        let msg = property.err().unwrap().to_string();
        assert!(msg.contains(
            "'app' can not be none, as it has been declared in nodes"
        ));
    }

    #[test]
    fn test_predefined_graph_app_in_connections_should_not_declared() {
        let property_str = include_str!(
            "test_data_embed/predefined_graph_app_in_connections_should_not_declared.json"
        );
        let property = Property::from_str(property_str);

        // The 'app' should not be declared, as not any node has declared it.
        assert!(property.is_err());
        println!("Error: {:?}", property);

        let msg = property.err().unwrap().to_string();
        assert!(msg.contains(
            "'app' should not be declared, as not any node has declared it"
        ));
    }

    #[test]
    fn test_predefined_graph_app_in_dest_not_all_declared() {
        let property_str = include_str!(
            "test_data_embed/predefined_graph_app_in_dest_not_all_declared.json"
        );
        let property = Property::from_str(property_str);

        // The 'app' can not be none, as it has been declared in nodes.
        assert!(property.is_err());
        println!("Error: {:?}", property);

        let msg = property.err().unwrap().to_string();
        assert!(msg.contains(
            "'app' can not be none, as it has been declared in nodes"
        ));
    }

    #[test]
    fn test_predefined_graph_app_in_dest_should_not_declared() {
        let property_str = include_str!(
            "test_data_embed/predefined_graph_app_in_dest_should_not_declared.json"
        );
        let property = Property::from_str(property_str);

        // The 'app' should not be declared, as not any node has declared it.
        assert!(property.is_err());
        println!("Error: {:?}", property);

        let msg = property.err().unwrap().to_string();
        assert!(msg.contains(
            "'app' should not be declared, as not any node has declared it"
        ));
    }

    #[test]
    fn test_graph_same_extension_in_two_section_of_connections() {
        let graph_str = include_str!(
            "test_data_embed/graph_same_extension_in_two_section_of_connections.json"
        );

        let graph = Graph::from_str(graph_str).unwrap();

        let result = graph.check_extension_uniqueness_in_connections();

        assert!(result.is_err());
        println!("Error: {:?}", result);

        let msg = result.err().unwrap().to_string();
        assert!(msg.contains("extension 'some_extension' is defined in connection[0] and connection[1]"));
    }

    #[test]
    fn test_graph_duplicated_cmd_name_in_one_connection() {
        let graph_str = include_str!(
            "test_data_embed/graph_duplicated_cmd_name_in_one_connection.json"
        );

        let graph = Graph::from_str(graph_str).unwrap();
        let result = graph.check_message_names();
        assert!(result.is_err());
        println!("Error: {:?}", result);

        let msg = result.err().unwrap().to_string();
        assert!(msg.contains("'hello' is defined in flow[0] and flow[1]"));
    }

    #[test]
    fn test_graph_messages_same_name_in_different_type_are_ok() {
        let graph_str = include_str!(
            "test_data_embed/graph_messages_same_name_in_different_type_are_ok.json"
        );

        let graph = Graph::from_str(graph_str).unwrap();
        let result = graph.check_message_names();
        assert!(result.is_ok());
    }

    #[test]
    fn test_graph_app_can_not_be_empty_string() {
        let graph_str = include_str!(
            "test_data_embed/graph_app_can_not_be_empty_string.json"
        );
        let graph = Graph::from_str(graph_str);

        // The 'app' can not be empty string.
        assert!(graph.is_err());
        println!("Error: {:?}", graph);

        let msg = graph.err().unwrap().to_string();
        assert!(msg.contains(constants::ERR_MSG_APP_IS_EMPTY));
    }

    #[test]
    fn test_graph_message_conversion_fixed_value() {
        let graph_str = include_str!(
            "test_data_embed/graph_message_conversion_fixed_value.json"
        );
        let graph = Graph::from_str(graph_str).unwrap();

        let connections = graph.connections.unwrap();
        let cmd = connections
            .first()
            .unwrap()
            .cmd
            .as_ref()
            .unwrap()
            .first()
            .unwrap();
        let msg_conversion =
            cmd.dest.first().unwrap().msg_conversion.as_ref().unwrap();
        let rules = &msg_conversion.msg.rules.rules;
        assert_eq!(rules.len(), 4);
        assert_eq!(rules[1].value.as_ref().unwrap().as_str().unwrap(), "hello");
        assert!(rules[2].value.as_ref().unwrap().as_bool().unwrap());
    }
}
