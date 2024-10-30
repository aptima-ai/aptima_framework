//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
use std::sync::{Arc, RwLock};

use actix_web::{http::StatusCode, test, web, App};

use ten_manager::{
    config::TmanConfig,
    dev_server::{
        graphs::{
            connections::{get_graph_connections, DevServerConnection},
            get_graphs, RespGraph,
        },
        response::{ApiResponse, ErrorResponse},
        DevServerState,
    },
};

#[actix_rt::test]
async fn test_cmd_dev_server_graphs_some_property_invalid() {
    let dev_server_state = DevServerState {
        base_dir: Some(
            "tests/test_data/cmd_dev_server_graphs_some_property_invalid"
                .to_string(),
        ),
        all_pkgs: None,
        tman_config: TmanConfig::default(),
    };

    let dev_server_state = Arc::new(RwLock::new(dev_server_state));
    let app = test::init_service(
        App::new()
            .app_data(web::Data::new(dev_server_state))
            .route("/api/dev-server/v1/graphs", web::get().to(get_graphs)),
    )
    .await;

    let req = test::TestRequest::get()
        .uri("/api/dev-server/v1/graphs")
        .to_request();
    let resp = test::call_service(&app, req).await;
    assert_eq!(resp.status(), StatusCode::NOT_FOUND);

    let body = test::read_body(resp).await;
    let body_str = std::str::from_utf8(&body).unwrap();
    let json: ErrorResponse = serde_json::from_str(body_str).unwrap();

    let pretty_json = serde_json::to_string_pretty(&json).unwrap();
    println!("Response body: {}", pretty_json);

    let root_cause = json.error.unwrap().message;
    assert!(root_cause
        .contains("Either all nodes should have 'app' declared, or none should, but not a mix of both"));
}

#[actix_rt::test]
async fn test_cmd_dev_server_graphs_app_property_not_exist() {
    let dev_server_state = DevServerState {
        base_dir: Some(
            "tests/test_data/cmd_dev_server_graphs_app_property_not_exist"
                .to_string(),
        ),
        all_pkgs: None,
        tman_config: TmanConfig::default(),
    };

    let dev_server_state = Arc::new(RwLock::new(dev_server_state));
    let app = test::init_service(
        App::new()
            .app_data(web::Data::new(dev_server_state))
            .route("/api/dev-server/v1/graphs", web::get().to(get_graphs)),
    )
    .await;

    let req = test::TestRequest::get()
        .uri("/api/dev-server/v1/graphs")
        .to_request();
    let resp = test::call_service(&app, req).await;
    assert_eq!(resp.status(), StatusCode::OK);

    let body = test::read_body(resp).await;
    let body_str = std::str::from_utf8(&body).unwrap();
    let json: ApiResponse<Vec<RespGraph>> =
        serde_json::from_str(body_str).unwrap();

    let pretty_json = serde_json::to_string_pretty(&json).unwrap();
    println!("Response body: {}", pretty_json);

    assert!(json.data.is_empty());
}

#[actix_rt::test]
async fn test_cmd_dev_server_connections_has_msg_conversion() {
    let dev_server_state = DevServerState {
        base_dir: Some(
            "tests/test_data/cmd_dev_server_connections_has_msg_conversion"
                .to_string(),
        ),
        all_pkgs: None,
        tman_config: TmanConfig::default(),
    };

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
    assert_eq!(resp.status(), StatusCode::OK);

    let body = test::read_body(resp).await;
    let body_str = std::str::from_utf8(&body).unwrap();
    let json: ApiResponse<Vec<DevServerConnection>> =
        serde_json::from_str(body_str).unwrap();

    let pretty_json = serde_json::to_string_pretty(&json).unwrap();
    println!("Response body: {}", pretty_json);

    let connections = &json.data;
    assert_eq!(connections.len(), 1);

    let connection = connections.first().unwrap();
    assert!(connection.cmd.is_some());

    let cmd = connection.cmd.as_ref().unwrap();
    assert_eq!(cmd.len(), 1);
}
