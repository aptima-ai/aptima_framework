//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
use std::sync::{Arc, RwLock};

use actix_web::{web, HttpResponse, Responder};
use serde::{Deserialize, Serialize};
use ten_rust::pkg_info::graph::{
    GraphConnection, GraphDestination, GraphMessageFlow,
};

use crate::dev_server::get_all_pkgs::get_all_pkgs;
use crate::dev_server::response::{ApiResponse, ErrorResponse, Status};
use crate::dev_server::DevServerState;
use ten_rust::pkg_info::pkg_type::PkgType;
use ten_rust::pkg_info::predefined_graphs::pkg_predefined_graphs_find;

#[derive(Serialize, Deserialize, Debug, PartialEq, Clone)]
pub struct DevServerConnection {
    pub app: String,
    pub extension_group: String,
    pub extension: String,

    #[serde(skip_serializing_if = "Option::is_none")]
    pub cmd: Option<Vec<DevServerMessageFlow>>,

    #[serde(skip_serializing_if = "Option::is_none")]
    pub data: Option<Vec<DevServerMessageFlow>>,

    #[serde(skip_serializing_if = "Option::is_none")]
    pub audio_frame: Option<Vec<DevServerMessageFlow>>,

    #[serde(skip_serializing_if = "Option::is_none")]
    pub video_frame: Option<Vec<DevServerMessageFlow>>,
}

impl From<GraphConnection> for DevServerConnection {
    fn from(conn: GraphConnection) -> Self {
        DevServerConnection {
            app: conn.get_app_uri().to_string(),
            extension_group: conn.extension_group,
            extension: conn.extension,

            cmd: conn.cmd.map(get_dev_server_msg_flow_from_property),

            data: conn.data.map(get_dev_server_msg_flow_from_property),

            audio_frame: conn
                .audio_frame
                .map(get_dev_server_msg_flow_from_property),

            video_frame: conn
                .video_frame
                .map(get_dev_server_msg_flow_from_property),
        }
    }
}

#[derive(Serialize, Deserialize, Debug, PartialEq, Clone)]
pub struct DevServerMessageFlow {
    pub name: String,
    pub dest: Vec<DevServerDestination>,
}

impl From<GraphMessageFlow> for DevServerMessageFlow {
    fn from(msg_flow: GraphMessageFlow) -> Self {
        DevServerMessageFlow {
            name: msg_flow.name,
            dest: get_dev_server_destination_from_property(msg_flow.dest),
        }
    }
}

fn get_dev_server_msg_flow_from_property(
    msg_flow: Vec<GraphMessageFlow>,
) -> Vec<DevServerMessageFlow> {
    if msg_flow.is_empty() {
        return vec![];
    }

    msg_flow.into_iter().map(|v| v.into()).collect()
}

#[derive(Serialize, Deserialize, Debug, PartialEq, Clone)]
pub struct DevServerDestination {
    pub app: String,
    pub extension_group: String,
    pub extension: String,
}

impl From<GraphDestination> for DevServerDestination {
    fn from(destination: GraphDestination) -> Self {
        DevServerDestination {
            app: destination.get_app_uri().to_string(),
            extension_group: destination.extension_group,
            extension: destination.extension,
        }
    }
}

fn get_dev_server_destination_from_property(
    destinations: Vec<GraphDestination>,
) -> Vec<DevServerDestination> {
    destinations.into_iter().map(|v| v.into()).collect()
}

pub async fn get_graph_connections(
    state: web::Data<Arc<RwLock<DevServerState>>>,
    path: web::Path<String>,
) -> impl Responder {
    let graph_name = path.into_inner();
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

    if let Some(pkgs) = &state.all_pkgs {
        if let Some(app_pkg) = pkgs
            .iter()
            .find(|pkg| pkg.pkg_identity.pkg_type == PkgType::App)
        {
            // If the app package has predefined graphs, find the one with the
            // specified graph_name.
            if let Some(predefined_graph) = pkg_predefined_graphs_find(
                app_pkg.get_predefined_graphs(),
                |g| g.name == graph_name,
            ) {
                // Convert the connections field to RespConnection.
                let connections: Option<_> =
                    predefined_graph.graph.connections.as_ref();
                let resp_connections: Vec<DevServerConnection> =
                    match connections {
                        Some(connections) => connections
                            .iter()
                            .map(|conn| conn.clone().into())
                            .collect(),
                        None => vec![],
                    };

                let response = ApiResponse {
                    status: Status::Ok,
                    data: resp_connections,
                    meta: None,
                };

                return HttpResponse::Ok().json(response);
            }
        }
    }

    let error_response = ErrorResponse {
        status: Status::Fail,
        message: "Graph not found".to_string(),
        error: None,
    };
    HttpResponse::NotFound().json(error_response)
}

#[cfg(test)]
mod tests {
    use std::vec;

    use actix_web::{test, App};

    use super::*;
    use crate::{
        config::TmanConfig, dev_server::mock::tests::inject_all_pkgs_for_mock,
    };
    use ten_rust::pkg_info::default_app_loc;

    #[actix_web::test]
    async fn test_get_connections_success() {
        let mut dev_server_state = DevServerState {
            base_dir: None,
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
            App::new().app_data(web::Data::new(dev_server_state)).route(
                "/api/dev-server/v1/graphs/{graph_name}/connections",
                web::get().to(get_graph_connections),
            ),
        )
        .await;

        let req = test::TestRequest::get()
            .uri("/api/dev-server/v1/graphs/default/connections")
            .to_request();
        let resp = test::call_service(&app, req).await;

        assert!(resp.status().is_success());

        let body = test::read_body(resp).await;
        let body_str = std::str::from_utf8(&body).unwrap();

        let connections: ApiResponse<Vec<DevServerConnection>> =
            serde_json::from_str(body_str).unwrap();

        let expected_connections = vec![DevServerConnection {
            app: default_app_loc(),
            extension_group: "extension_group_1".to_string(),
            extension: "extension_1".to_string(),
            cmd: Some(vec![DevServerMessageFlow {
                name: "hello_world".to_string(),
                dest: vec![DevServerDestination {
                    app: default_app_loc(),
                    extension_group: "extension_group_1".to_string(),
                    extension: "extension_2".to_string(),
                }],
            }]),
            data: None,
            audio_frame: None,
            video_frame: None,
        }];

        assert_eq!(connections.data, expected_connections);
        assert!(!connections.data.is_empty());

        let json: serde_json::Value = serde_json::from_str(body_str).unwrap();
        let pretty_json = serde_json::to_string_pretty(&json).unwrap();
        println!("Response body: {}", pretty_json);
    }

    #[actix_web::test]
    async fn test_get_connections_have_all_data_type() {
        let mut dev_server_state = DevServerState {
            base_dir: None,
            all_pkgs: None,
            tman_config: TmanConfig::default(),
        };

        // The first item is 'manifest.json', and the second item is
        // 'property.json'.
        let all_pkgs_json = vec![
            (
                include_str!("test_data_embed/get_connections_have_all_data_type/app_manifest.json")
                    .to_string(),
                include_str!("test_data_embed/get_connections_have_all_data_type/app_property.json")
                    .to_string(),
            ),
            (
                include_str!("test_data_embed/get_connections_have_all_data_type/extension_addon_1_manifest.json")
                    .to_string(),
                "{}".to_string(),
            ),
            (
                include_str!("test_data_embed/get_connections_have_all_data_type/extension_addon_2_manifest.json")
                    .to_string(),
                "{}".to_string(),
            ),
        ];

        let inject_ret =
            inject_all_pkgs_for_mock(&mut dev_server_state, all_pkgs_json);
        assert!(inject_ret.is_ok());

        let dev_server_state = Arc::new(RwLock::new(dev_server_state));
        let app = test::init_service(
            App::new().app_data(web::Data::new(dev_server_state)).route(
                "/api/dev-server/v1/graphs/{graph_name}/connections",
                web::get().to(get_graph_connections),
            ),
        )
        .await;

        let req = test::TestRequest::get()
            .uri("/api/dev-server/v1/graphs/default/connections")
            .to_request();
        let resp = test::call_service(&app, req).await;

        assert!(resp.status().is_success());

        let body = test::read_body(resp).await;
        let body_str = std::str::from_utf8(&body).unwrap();

        let connections: ApiResponse<Vec<DevServerConnection>> =
            serde_json::from_str(body_str).unwrap();

        let expected_connections = vec![DevServerConnection {
            app: default_app_loc(),
            extension_group: "extension_group_1".to_string(),
            extension: "extension_1".to_string(),
            cmd: Some(vec![DevServerMessageFlow {
                name: "hello_world".to_string(),
                dest: vec![DevServerDestination {
                    app: default_app_loc(),
                    extension_group: "extension_group_1".to_string(),
                    extension: "extension_2".to_string(),
                }],
            }]),
            data: Some(vec![DevServerMessageFlow {
                name: "data".to_string(),
                dest: vec![DevServerDestination {
                    app: default_app_loc(),
                    extension_group: "extension_group_1".to_string(),
                    extension: "extension_2".to_string(),
                }],
            }]),
            audio_frame: Some(vec![DevServerMessageFlow {
                name: "pcm".to_string(),
                dest: vec![DevServerDestination {
                    app: default_app_loc(),
                    extension_group: "extension_group_1".to_string(),
                    extension: "extension_2".to_string(),
                }],
            }]),
            video_frame: Some(vec![DevServerMessageFlow {
                name: "image".to_string(),
                dest: vec![DevServerDestination {
                    app: default_app_loc(),
                    extension_group: "extension_group_1".to_string(),
                    extension: "extension_2".to_string(),
                }],
            }]),
        }];

        assert_eq!(connections.data, expected_connections);
        assert!(!connections.data.is_empty());

        let json: serde_json::Value = serde_json::from_str(body_str).unwrap();
        let pretty_json = serde_json::to_string_pretty(&json).unwrap();
        println!("Response body: {}", pretty_json);
    }
}
