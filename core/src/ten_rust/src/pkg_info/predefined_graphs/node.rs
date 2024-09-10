//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
use std::str::FromStr;

use crate::pkg_info::{graph::GraphNode, pkg_type::PkgType, PkgInfo};

#[derive(Debug, Clone)]
pub struct PkgNode {
    pub node_type: PkgType,
    pub name: String,
    pub addon: String,

    // If its an extension group node, it does not contain a extension_group
    // field.
    pub extension_group: Option<String>,

    pub app: String,

    pub property: Option<serde_json::Value>,

    pub pkg_info: Option<PkgInfo>,
}

impl From<GraphNode> for PkgNode {
    fn from(manifest_node: GraphNode) -> Self {
        PkgNode {
            node_type: PkgType::from_str(&manifest_node.node_type).unwrap(),
            name: manifest_node.name,
            addon: manifest_node.addon,
            extension_group: manifest_node.extension_group,
            app: manifest_node.app,
            property: manifest_node.property,
            pkg_info: None,
        }
    }
}
