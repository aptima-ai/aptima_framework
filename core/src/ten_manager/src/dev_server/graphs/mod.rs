//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
pub mod connections;
pub mod nodes;
pub mod update;

use std::sync::{Arc, RwLock};

use actix_web::{web, HttpResponse, Responder};
use serde::{Deserialize, Serialize};

use super::{
    get_all_pkgs::get_all_pkgs,
    response::{ApiResponse, ErrorResponse, Status},
    DevServerState,
};
use ten_rust::pkg_info::pkg_type::PkgType;

#[derive(Serialize, Deserialize, Debug, PartialEq)]
struct RespGraph {
    name: String,
    auto_start: bool,
}

pub async fn get_graphs(
    state: web::Data<Arc<RwLock<DevServerState>>>,
) -> Result<impl Responder, actix_web::Error> {
    let mut state = state.write().unwrap();

    // Fetch all packages if not already done.
    if let Err(err) = get_all_pkgs(&mut state) {
        let error_response = ErrorResponse {
            status: Status::Fail,
            message: format!("Error fetching packages: {}", err),
            error: None,
        };
        return Ok(HttpResponse::NotFound().json(error_response));
    }

    if let Some(all_pkgs) = &state.all_pkgs {
        if let Some(app_pkg) = all_pkgs
            .iter()
            .find(|pkg| pkg.pkg_identity.pkg_type == PkgType::App)
        {
            let graphs: Vec<RespGraph> = app_pkg
                .predefined_graphs
                .iter()
                .map(|graph| RespGraph {
                    name: graph.name.clone(),
                    auto_start: graph.auto_start,
                })
                .collect();

            let response = ApiResponse {
                status: Status::Ok,
                data: graphs,
                meta: None,
            };

            Ok(HttpResponse::Ok().json(response))
        } else {
            let error_response = ErrorResponse {
                status: Status::Fail,
                message: "Failed to find any app packages".to_string(),
                error: None,
            };
            Ok(HttpResponse::NotFound().json(error_response))
        }
    } else {
        let error_response = ErrorResponse {
            status: Status::Fail,
            message: "All packages not available".to_string(),
            error: None,
        };
        Ok(HttpResponse::NotFound().json(error_response))
    }
}

#[cfg(test)]
mod tests {
    use crate::{
        config::TmanConfig, dev_server::mock::tests::inject_all_pkgs_for_mock,
    };

    use super::*;
    use actix_web::{test, App};

    #[actix_web::test]
    async fn test_get_graphs_success() {
        let mut dev_server_state = DevServerState {
            base_dir: None,
            all_pkgs: None,
            tman_config: TmanConfig::default(),
        };

        let all_pkgs_json = vec![
            (
                include_str!(
                    "../test_data/large_response_source_app_manifest.json"
                )
                .to_string(),
                include_str!(
                    "../test_data/large_response_source_app_property.json"
                )
                .to_string(),
            ),
            (
                include_str!("../test_data/extension_addon_1_manifest.json")
                    .to_string(),
                "{}".to_string(),
            ),
            (
                include_str!("../test_data/extension_addon_2_manifest.json")
                    .to_string(),
                "{}".to_string(),
            ),
            (
                include_str!("../test_data/extension_addon_3_manifest.json")
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
                .app_data(web::Data::new(dev_server_state))
                .route("/api/dev-server/v1/graphs", web::get().to(get_graphs)),
        )
        .await;

        let req = test::TestRequest::get()
            .uri("/api/dev-server/v1/graphs")
            .to_request();
        let resp = test::call_service(&app, req).await;

        assert!(resp.status().is_success());

        let body = test::read_body(resp).await;
        let body_str = std::str::from_utf8(&body).unwrap();

        let graphs: ApiResponse<Vec<RespGraph>> =
            serde_json::from_str(body_str).unwrap();

        let expected_graphs = vec![
            RespGraph {
                name: "0".to_string(),
                auto_start: true,
            },
            RespGraph {
                name: "addon_not_found".to_string(),
                auto_start: false,
            },
        ];

        assert_eq!(graphs.data, expected_graphs);

        let json: serde_json::Value = serde_json::from_str(body_str).unwrap();
        let pretty_json = serde_json::to_string_pretty(&json).unwrap();
        println!("Response body: {}", pretty_json);
    }

    #[actix_web::test]
    async fn test_get_graphs_no_app_package() {
        let dev_server_state = Arc::new(RwLock::new(DevServerState {
            base_dir: None,
            all_pkgs: Some(vec![]),
            tman_config: TmanConfig::default(),
        }));

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

        assert!(resp.status().is_client_error());
        println!("Response body: {}", resp.status());

        let body = test::read_body(resp).await;
        let body_str = std::str::from_utf8(&body).unwrap();

        let error: ErrorResponse = serde_json::from_str(body_str).unwrap();

        assert_eq!(error.message, "Failed to find any app packages");
    }
}
