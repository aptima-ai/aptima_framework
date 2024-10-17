//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
use std::{collections::HashMap, path::Path, str::FromStr};

use ten_rust::pkg_info::{
    get_all_existed_pkgs_info_of_app, graph::Graph, localhost,
    pkg_type::PkgType, property::predefined_graph::PropertyPredefinedGraph,
    PkgInfo,
};

#[test]
fn test_graph_check_extension_not_installed() {
    let app_dir = "tests/test_data/graph_check_extension_not_installed";
    let pkg_infos =
        get_all_existed_pkgs_info_of_app(Path::new(app_dir)).unwrap();
    assert!(!pkg_infos.is_empty());

    let app_pkg_info = pkg_infos
        .iter()
        .filter(|pkg| pkg.pkg_identity.pkg_type == PkgType::App)
        .last();
    let app_pkg = app_pkg_info.unwrap();
    let predefined_graphs = app_pkg.get_predefined_graphs().unwrap();
    let pkg_graph = predefined_graphs.first().unwrap();
    let predefined_graph: PropertyPredefinedGraph = pkg_graph.clone();
    let graph = &predefined_graph.graph;

    let mut pkg_info_map: HashMap<String, Vec<PkgInfo>> = HashMap::new();
    pkg_info_map.insert(localhost(), pkg_infos);

    let result = graph.check_if_nodes_have_installed(&pkg_info_map, false);
    assert!(result.is_err());
    println!("Error: {:?}", result.err().unwrap());
}

#[test]
fn test_graph_check_predefined_graph_success() {
    let app_dir = "tests/test_data/graph_check_predefined_graph_success";
    let pkg_infos =
        get_all_existed_pkgs_info_of_app(Path::new(app_dir)).unwrap();
    assert!(!pkg_infos.is_empty());

    let app_pkg_info = pkg_infos
        .iter()
        .filter(|pkg| pkg.pkg_identity.pkg_type == PkgType::App)
        .last();
    let app_pkg = app_pkg_info.unwrap();
    let predefined_graphs = app_pkg.get_predefined_graphs().unwrap();
    let pkg_graph = predefined_graphs.first().unwrap();
    let predefined_graph: PropertyPredefinedGraph = pkg_graph.clone();
    let graph = &predefined_graph.graph;

    let mut pkg_info_map: HashMap<String, Vec<PkgInfo>> = HashMap::new();
    pkg_info_map.insert(localhost(), pkg_infos);

    let result = graph.check(&pkg_info_map);
    assert!(result.is_ok());
}

#[test]
fn test_graph_check_all_msgs_schema_incompatible() {
    let app_dir = "tests/test_data/graph_check_all_msgs_schema_incompatible";
    let pkg_infos =
        get_all_existed_pkgs_info_of_app(Path::new(app_dir)).unwrap();
    assert!(!pkg_infos.is_empty());

    let app_pkg_info = pkg_infos
        .iter()
        .filter(|pkg| pkg.pkg_identity.pkg_type == PkgType::App)
        .last();
    let app_pkg = app_pkg_info.unwrap();
    let predefined_graphs = app_pkg.get_predefined_graphs().unwrap();
    let pkg_graph = predefined_graphs.first().unwrap();
    let predefined_graph: PropertyPredefinedGraph = pkg_graph.clone();
    let graph = &predefined_graph.graph;

    let mut pkg_info_map: HashMap<String, Vec<PkgInfo>> = HashMap::new();
    pkg_info_map.insert(localhost(), pkg_infos);

    let result = graph.check(&pkg_info_map);
    assert!(result.is_err());
    println!("Error: {:?}", result.err().unwrap());
}

#[test]
fn test_graph_check_single_app() {
    let app_dir = "tests/test_data/graph_check_single_app";
    let pkg_infos =
        get_all_existed_pkgs_info_of_app(Path::new(app_dir)).unwrap();
    assert!(!pkg_infos.is_empty());

    let graph_str = include_str!("test_data/graph_check_single_app/graph.json");
    let graph = Graph::from_str(graph_str).unwrap();

    let mut pkg_info_map: HashMap<String, Vec<PkgInfo>> = HashMap::new();
    pkg_info_map.insert("http://localhost:8001".to_string(), pkg_infos);

    // The schema of 'ext_c' is not found, but it's OK because we only check
    // for the app 'http://localhost:8001'.
    let result = graph.check_for_single_app(&pkg_info_map);
    assert!(result.is_ok());
}

#[test]
fn test_graph_check_single_app_schema_incompatible() {
    let app_dir = "tests/test_data/graph_check_single_app_schema_incompatible";
    let pkg_infos =
        get_all_existed_pkgs_info_of_app(Path::new(app_dir)).unwrap();
    assert!(!pkg_infos.is_empty());

    let graph_str = include_str!(
        "test_data/graph_check_single_app_schema_incompatible/graph.json"
    );
    let graph = Graph::from_str(graph_str).unwrap();

    let mut pkg_info_map: HashMap<String, Vec<PkgInfo>> = HashMap::new();
    pkg_info_map.insert("http://localhost:8001".to_string(), pkg_infos);

    let result = graph.check_for_single_app(&pkg_info_map);

    // Schema incompatible to [extension_group: some_group, extension: ext_b].
    assert!(result.is_err());
    println!("Error: {:?}", result.err().unwrap());
}
