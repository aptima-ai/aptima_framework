//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
pub mod connection;
pub mod extension;
pub mod node;

use anyhow::Result;

use crate::pkg_info::{
    graph::Graph,
    property::{predefined_graph::PropertyPredefinedGraph, Property},
};
use connection::PkgConnection;
use node::PkgNode;

pub fn get_pkg_predefined_graphs_from_property(
    property: &Property,
) -> Result<Vec<PkgPredefinedGraph>> {
    let mut graphs = Vec::new();

    if let Some(ten) = &property._ten {
        if let Some(predefined_graphs) = &ten.predefined_graphs {
            for property_predefined_graph in predefined_graphs {
                graphs.push(PkgPredefinedGraph {
                    name: property_predefined_graph.name.clone(),
                    auto_start: property_predefined_graph
                        .auto_start
                        .unwrap_or(false),
                    nodes: property_predefined_graph
                        .graph
                        .nodes
                        .iter()
                        .cloned()
                        .map(|m| m.into())
                        .collect(),
                    connections: match &property_predefined_graph
                        .graph
                        .connections
                    {
                        Some(connections) => connections
                            .iter()
                            .cloned()
                            .map(|m| m.into())
                            .collect(),
                        None => Vec::new(),
                    },
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
    connections: &Vec<PkgConnection>,
) -> Result<PkgPredefinedGraph> {
    Ok(PkgPredefinedGraph {
        name: graph_name.to_owned(),
        auto_start,
        nodes: nodes.to_vec(),
        connections: connections.to_vec(),
    })
}

#[derive(Debug, Clone)]
pub struct PkgPredefinedGraph {
    pub name: String,
    pub auto_start: bool,
    pub nodes: Vec<PkgNode>,
    pub connections: Vec<PkgConnection>,
}

impl From<PropertyPredefinedGraph> for PkgPredefinedGraph {
    fn from(property_graph: PropertyPredefinedGraph) -> Self {
        PkgPredefinedGraph {
            name: property_graph.name,
            auto_start: property_graph.auto_start.unwrap_or(false),
            nodes: property_graph
                .graph
                .nodes
                .into_iter()
                .map(|v| v.into())
                .collect(),
            connections: match property_graph.graph.connections {
                Some(connections) => {
                    connections.into_iter().map(|v| v.into()).collect()
                }
                None => Vec::new(),
            },
        }
    }
}

impl From<PkgPredefinedGraph> for PropertyPredefinedGraph {
    fn from(pkg_predefined_graph: PkgPredefinedGraph) -> Self {
        PropertyPredefinedGraph {
            name: pkg_predefined_graph.name.clone(),
            auto_start: Some(pkg_predefined_graph.auto_start),
            graph: Graph {
                nodes: pkg_predefined_graph
                    .nodes
                    .iter()
                    .map(|n| n.clone().into())
                    .collect(),
                connections: Some(
                    pkg_predefined_graph
                        .connections
                        .iter()
                        .map(|c| c.clone().into())
                        .collect(),
                ),
            },
        }
    }
}

pub fn get_property_predefined_graphs_from_pkg(
    pkg_predefined_graphs: Vec<PkgPredefinedGraph>,
) -> Vec<PropertyPredefinedGraph> {
    pkg_predefined_graphs
        .into_iter()
        .map(|v| v.into())
        .collect()
}
