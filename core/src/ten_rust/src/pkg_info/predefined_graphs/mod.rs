//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
pub mod extension;

use anyhow::Result;

use crate::pkg_info::{
    graph::Graph, property::predefined_graph::PredefinedGraph,
};

use super::graph::{GraphConnection, GraphNode};

pub fn pkg_predefined_graphs_find<F>(
    pkg_predefined_graphs: Option<&Vec<PredefinedGraph>>,
    predicate: F,
) -> Option<&PredefinedGraph>
where
    F: Fn(&&PredefinedGraph) -> bool,
{
    match pkg_predefined_graphs {
        None => None,
        Some(pkg_predefined_graphs) => {
            pkg_predefined_graphs.iter().find(predicate)
        }
    }
}

pub fn get_pkg_predefined_graph_from_nodes_and_connections(
    graph_name: &str,
    auto_start: bool,
    nodes: &[GraphNode],
    connections: &[GraphConnection],
) -> Result<PredefinedGraph> {
    Ok(PredefinedGraph {
        name: graph_name.to_string(),
        auto_start: Some(auto_start),
        graph: Graph {
            nodes: nodes.to_vec(),
            connections: Some(connections.to_vec()),
        },
    })
}
