//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
pub mod extension;
pub mod node;

use anyhow::Result;

use crate::pkg_info::{
    graph::Graph,
    property::{predefined_graph::PropertyPredefinedGraph, Property},
};
use node::PkgNode;

use super::graph::GraphConnection;

#[derive(Debug, Clone)]
pub struct PkgPredefinedGraph {
    pub prop_predefined_graph: PropertyPredefinedGraph,

    // TODO(Liu):
    // * Using GraphNode instead.
    // * Add Vec<PkgInfo> to store the extra pkg info for nodes in this graph.
    pub nodes: Vec<PkgNode>,
}

pub fn get_pkg_predefined_graphs_from_property(
    property: &Property,
) -> Result<Vec<PkgPredefinedGraph>> {
    let mut graphs = Vec::new();

    if let Some(ten) = &property._ten {
        if let Some(predefined_graphs) = &ten.predefined_graphs {
            for property_predefined_graph in predefined_graphs {
                graphs.push(PkgPredefinedGraph {
                    nodes: property_predefined_graph
                        .graph
                        .nodes
                        .iter()
                        .cloned()
                        .map(|m| m.into())
                        .collect(),
                    prop_predefined_graph: property_predefined_graph.clone(),
                });
            }
        }
    }

    Ok(graphs)
}

pub fn pkg_predefined_graphs_find<F>(
    pkg_predefined_graphs: &[PkgPredefinedGraph],
    predicate: F,
) -> Option<&PkgPredefinedGraph>
where
    F: Fn(&&PkgPredefinedGraph) -> bool,
{
    pkg_predefined_graphs.iter().find(predicate)
}

pub fn get_pkg_predefined_graph_from_nodes_and_connections(
    graph_name: &str,
    auto_start: bool,
    nodes: &Vec<PkgNode>,
    connections: &Vec<GraphConnection>,
) -> Result<PkgPredefinedGraph> {
    Ok(PkgPredefinedGraph {
        prop_predefined_graph: PropertyPredefinedGraph {
            name: graph_name.to_string(),
            auto_start: Some(auto_start),
            graph: Graph {
                nodes: nodes.iter().map(|n| n.clone().into()).collect(),
                connections: Some(
                    connections.iter().map(|c| c.clone().into()).collect(),
                ),
            },
        },
        nodes: nodes.to_vec(),
    })
}
