//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
use std::sync::{Arc, RwLock};

use actix_web::{web, HttpResponse, Responder};
use serde::{Deserialize, Serialize};

use super::{connections::DevServerConnection, nodes::DevServerExtension};
use crate::dev_server::response::{ApiResponse, ErrorResponse, Status};
use crate::dev_server::{get_all_pkgs::get_all_pkgs, DevServerState};
use ten_rust::pkg_info::pkg_type::PkgType;
use ten_rust::pkg_info::predefined_graphs::get_pkg_predefined_graph_from_nodes_and_connections;

#[derive(Deserialize, Debug, PartialEq, Clone)]
pub struct GraphUpdateRequest {
    pub auto_start: bool,
    pub nodes: Vec<DevServerExtension>,
    pub connections: Vec<DevServerConnection>,
}

#[derive(Serialize, Debug, PartialEq)]
pub struct GraphUpdateResponse {
    pub success: bool,
}

pub async fn update_graph(
    req: web::Path<String>,
    body: web::Json<GraphUpdateRequest>,
    state: web::Data<Arc<RwLock<DevServerState>>>,
) -> impl Responder {
    let graph_name = req.into_inner();
    let mut state = state.write().unwrap();

    // Fetch all packages if not already done.
    if let Err(err) = get_all_pkgs(&mut state) {
        let error_response = ErrorResponse {
            status: Status::Fail,
            message: format!("Error fetching packages: {}", err),
            error: None,
        };
        return HttpResponse::NotFound().json(error_response);
    }

    if let Some(pkgs) = &mut state.all_pkgs {
        if let Some(app_pkg) = pkgs
            .iter_mut()
            .find(|pkg| pkg.pkg_identity.pkg_type == PkgType::App)
        {
            let new_graph =
                match get_pkg_predefined_graph_from_nodes_and_connections(
                    &graph_name,
                    body.auto_start,
                    &body.nodes.clone().into_iter().map(|v| v.into()).collect(),
                    &body
                        .connections
                        .clone()
                        .into_iter()
                        .map(|v| v.into())
                        .collect(),
                ) {
                    Ok(graph) => graph,
                    Err(err) => {
                        let error_response = ErrorResponse {
                            status: Status::Fail,
                            message: format!("Invalid input data: {}", err),
                            error: None,
                        };
                        return HttpResponse::NotFound().json(error_response);
                    }
                };

            // TODO(Liu): Add graph check.
            app_pkg.update_predefined_graph(&new_graph);

            let response = ApiResponse {
                status: Status::Ok,
                data: GraphUpdateResponse { success: true },
                meta: None,
            };

            HttpResponse::Ok().json(response)
        } else {
            let error_response = ErrorResponse {
                status: Status::Fail,
                message: "Failed to find app package.".to_string(),
                error: None,
            };
            HttpResponse::NotFound().json(error_response)
        }
    } else {
        let error_response = ErrorResponse {
            status: Status::Fail,
            message: "Package information is missing".to_string(),
            error: None,
        };
        HttpResponse::NotFound().json(error_response)
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::{
        config::TmanConfig, dev_server::mock::tests::inject_all_pkgs_for_mock,
    };
    use actix_web::{test, App};
    use serde_json::Value;
    use std::{env, fs};

    #[actix_web::test]
    async fn test_update_graph_success() {
        // Get current working directory when testing.
        let exe_path =
            env::current_exe().expect("Failed to get the executable path");
        let test_dir = exe_path
            .parent()
            .expect("Failed to get the parent directory");

        env::set_current_dir(test_dir)
            .expect("Failed to set current directory");

        let test_data_dir = test_dir.join("test_data");
        fs::create_dir_all(&test_data_dir)
            .expect("Failed to create test_data directory");

        let mut dev_server_state = DevServerState {
            base_dir: Some(test_data_dir.to_string_lossy().to_string()),
            all_pkgs: None,
            tman_config: TmanConfig::default(),
        };

        let all_pkgs_json = vec![
            (
                include_str!("test_data_embed/app_manifest.json").to_string(),
                include_str!("test_data_embed/app_property.json").to_string(),
            ),
            (
                include_str!("test_data_embed/extension_addon_1_manifest.json")
                    .to_string(),
                "{}".to_string(),
            ),
            (
                include_str!("test_data_embed/extension_addon_2_manifest.json")
                    .to_string(),
                "{}".to_string(),
            ),
            (
                include_str!("test_data_embed/extension_addon_3_manifest.json")
                    .to_string(),
                "{}".to_string(),
            ),
        ];

        let inject_ret =
            inject_all_pkgs_for_mock(&mut dev_server_state, all_pkgs_json);
        assert!(inject_ret.is_ok());

        let dev_server_state = Arc::new(RwLock::new(dev_server_state));

        let app = test::init_service(
            App::new()
                .app_data(web::Data::new(dev_server_state.clone()))
                .route(
                    "/api/dev-server/v1/graphs/{graph_name}",
                    web::put().to(update_graph),
                ),
        )
        .await;

        let input_data: Value = serde_json::from_str(include_str!(
            "test_data_embed/update_graph_success/input_data.json"
        ))
        .unwrap();

        let req = test::TestRequest::put()
            .uri("/api/dev-server/v1/graphs/default")
            .set_json(input_data)
            .to_request();
        let resp = test::call_service(&app, req).await;

        assert!(resp.status().is_success());

        match &dev_server_state.clone().read().unwrap().all_pkgs {
            Some(pkgs) => {
                let app_pkg = pkgs
                    .iter()
                    .find(|pkg| pkg.pkg_identity.pkg_type == PkgType::App)
                    .unwrap();

                let predefined_graphs =
                    app_pkg.get_predefined_graphs().unwrap();
                let predefined_graph = predefined_graphs
                    .iter()
                    .find(|g| g.name == "default")
                    .unwrap();

                assert!(!predefined_graph.auto_start.unwrap());
                assert_eq!(predefined_graph.graph.nodes.len(), 2);
                assert_eq!(
                    predefined_graph.graph.connections.as_ref().unwrap().len(),
                    1
                );

                let property = app_pkg.property.as_ref().unwrap();
                let property_predefined_graphs = property
                    ._ten
                    .as_ref()
                    .unwrap()
                    .predefined_graphs
                    .as_ref()
                    .unwrap();
                let property_predefined_graph = property_predefined_graphs
                    .iter()
                    .find(|g| g.name == "default")
                    .unwrap();

                assert_eq!(property_predefined_graph.auto_start, Some(false));
                assert_eq!(property_predefined_graph.graph.nodes.len(), 2);
                assert_eq!(
                    property_predefined_graph
                        .graph
                        .connections
                        .as_ref()
                        .unwrap()
                        .len(),
                    1
                );
            }
            None => panic!("Failed to get all packages"),
        }
    }
}
