//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
use std::{collections::HashMap, path::Path};

use ten_rust::pkg_info::{
    default_app_loc, get_all_existed_pkgs_info_of_app, pkg_type::PkgType,
    property::predefined_graph::PropertyPredefinedGraph, PkgInfo,
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
    let pkg_graph = app_pkg.predefined_graphs.first().unwrap();
    let predefined_graph: PropertyPredefinedGraph =
        pkg_graph.prop_predefined_graph.clone();
    let graph = &predefined_graph.graph;

    let mut pkg_info_map: HashMap<String, Vec<PkgInfo>> = HashMap::new();
    pkg_info_map.insert(default_app_loc(), pkg_infos);

    let result = graph.check_if_nodes_have_installed(&pkg_info_map);
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
    let pkg_graph = app_pkg.predefined_graphs.first().unwrap();
    let predefined_graph: PropertyPredefinedGraph =
        pkg_graph.prop_predefined_graph.clone();
    let graph = &predefined_graph.graph;

    let mut pkg_info_map: HashMap<String, Vec<PkgInfo>> = HashMap::new();
    pkg_info_map.insert(default_app_loc(), pkg_infos);

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
    let pkg_graph = app_pkg.predefined_graphs.first().unwrap();
    let predefined_graph: PropertyPredefinedGraph =
        pkg_graph.prop_predefined_graph.clone();
    let graph = &predefined_graph.graph;

    let mut pkg_info_map: HashMap<String, Vec<PkgInfo>> = HashMap::new();
    pkg_info_map.insert(default_app_loc(), pkg_infos);

    let result = graph.check(&pkg_info_map);
    assert!(result.is_err());
    println!("Error: {:?}", result.err().unwrap());
}
