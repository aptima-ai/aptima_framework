//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
use std::sync::{Arc, RwLock};

use actix_web::{http::StatusCode, test, web, App};

use axis_manager::{
    config::TmanConfig,
    designer::{
        graphs::{
            connections::{get_graph_connections, DesignerConnection},
            get_graphs, RespGraph,
        },
        response::{ApiResponse, ErrorResponse},
        DesignerState,
    },
};

#[actix_rt::test]
async fn test_cmd_designer_graphs_some_property_invalid() {
    let designer_state = DesignerState {
        base_dir: Some(
            "tests/test_data/cmd_designer_graphs_some_property_invalid"
                .to_string(),
        ),
        all_pkgs: None,
        tman_config: TmanConfig::default(),
    };

    let designer_state = Arc::new(RwLock::new(designer_state));
    let app = test::init_service(
        App::new()
            .app_data(web::Data::new(designer_state))
            .route("/api/designer/v1/graphs", web::get().to(get_graphs)),
    )
    .await;

    let req = test::TestRequest::get()
        .uri("/api/designer/v1/graphs")
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
async fn test_cmd_designer_graphs_app_property_not_exist() {
    let designer_state = DesignerState {
        base_dir: Some(
            "tests/test_data/cmd_designer_graphs_app_property_not_exist"
                .to_string(),
        ),
        all_pkgs: None,
        tman_config: TmanConfig::default(),
    };

    let designer_state = Arc::new(RwLock::new(designer_state));
    let app = test::init_service(
        App::new()
            .app_data(web::Data::new(designer_state))
            .route("/api/designer/v1/graphs", web::get().to(get_graphs)),
    )
    .await;

    let req = test::TestRequest::get()
        .uri("/api/designer/v1/graphs")
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
async fn test_cmd_designer_connections_has_msg_conversion() {
    let designer_state = DesignerState {
        base_dir: Some(
            "tests/test_data/cmd_designer_connections_has_msg_conversion"
                .to_string(),
        ),
        all_pkgs: None,
        tman_config: TmanConfig::default(),
    };

    let designer_state = Arc::new(RwLock::new(designer_state));
    let app = test::init_service(
        App::new().app_data(web::Data::new(designer_state)).route(
            "/api/designer/v1/graphs/{graph_name}/connections",
            web::get().to(get_graph_connections),
        ),
    )
    .await;

    let req = test::TestRequest::get()
        .uri("/api/designer/v1/graphs/default/connections")
        .to_request();
    let resp = test::call_service(&app, req).await;
    assert_eq!(resp.status(), StatusCode::OK);

    let body = test::read_body(resp).await;
    let body_str = std::str::from_utf8(&body).unwrap();
    let json: ApiResponse<Vec<DesignerConnection>> =
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
