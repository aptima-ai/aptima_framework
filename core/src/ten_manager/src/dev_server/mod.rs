//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
mod addons;
mod common;
mod frontend;
mod get_all_pkgs;
pub mod graphs;
mod manifest;
mod messages;
mod mock;
mod packages;
mod property;
pub mod response;
mod version;

use std::sync::{Arc, RwLock};

use actix_web::{web, HttpRequest, HttpResponse, Responder};
use mime_guess::from_path;

use frontend::Asset;
use ten_rust::pkg_info::PkgInfo;

use super::config::TmanConfig;
use version::get_version;

pub struct DevServerState {
    pub base_dir: Option<String>,
    pub all_pkgs: Option<Vec<PkgInfo>>,
    pub tman_config: TmanConfig,
}

async fn get_frontend_asset(req: HttpRequest) -> impl Responder {
    let path = req.path().trim_start_matches('/').to_owned();

    if path.is_empty() {
        // If the root path is requested, return `index.html`.
        match Asset::get("index.html") {
            Some(content) => HttpResponse::Ok()
                .content_type("text/html")
                .body(content.data.into_owned()),
            None => HttpResponse::NotFound().body("404 Not Found"),
        }
    } else {
        match Asset::get(&path) {
            Some(content) => {
                let mime = from_path(&path).first_or_octet_stream();
                HttpResponse::Ok()
                    .content_type(mime.as_ref())
                    .body(content.data.into_owned())
            }
            // If the file is not found, return `index.html` to support React
            // Router.
            None => match Asset::get("index.html") {
                Some(content) => HttpResponse::Ok()
                    .content_type("text/html")
                    .body(content.data.into_owned()),
                None => HttpResponse::NotFound().body("404 Not Found"),
            },
        }
    }
}

pub fn configure_routes(
    cfg: &mut web::ServiceConfig,
    state: web::Data<Arc<RwLock<DevServerState>>>,
) {
    cfg.app_data(state.clone())
        .route("/api/dev-server/v1/version", web::get().to(get_version))
        .route(
            "/api/dev-server/v1/addons/extensions",
            web::get().to(addons::extensions::get_extension_addons),
        )
        .route(
            "/api/dev-server/v1/packages/reload",
            web::post().to(packages::reload::clear_and_reload_pkgs),
        )
        .route(
            "/api/dev-server/v1/graphs",
            web::get().to(graphs::get_graphs),
        )
        .route(
            "/api/dev-server/v1/graphs/{graph_name}/nodes",
            web::get().to(graphs::nodes::get_graph_nodes),
        )
        .route(
            "/api/dev-server/v1/graphs/{graph_name}/connections",
            web::get().to(graphs::connections::get_graph_connections),
        )
        .route(
            "/api/dev-server/v1/graphs/{graph_name}",
            web::put().to(graphs::update::update_graph),
        )
        .route(
            "/api/dev-server/v1/manifest",
            web::put().to(manifest::dump::dump_manifest),
        )
        .route(
            "/api/dev-server/v1/manifest/check",
            web::get().to(manifest::check::check_manifest),
        )
        .route(
            "/api/dev-server/v1/property",
            web::put().to(property::dump::dump_property),
        )
        .route(
            "/api/dev-server/v1/property/check",
            web::get().to(property::check::check_property),
        )
        .route(
            "/api/dev-server/v1/messages/compatible",
            web::post().to(messages::compatible::get_compatible_messages),
        )
        .default_service(web::route().to(get_frontend_asset));
}

#[cfg(test)]
mod tests {
    use super::*;
    use actix_web::{http::StatusCode, test, App};

    #[actix_web::test]
    async fn test_undefined_endpoint() {
        // Initialize the DevServerState.
        let state = web::Data::new(Arc::new(RwLock::new(DevServerState {
            base_dir: None,
            all_pkgs: None,
            tman_config: TmanConfig::default(),
        })));

        // Create the App with the routes configured.
        let app = test::init_service(
            App::new().configure(|cfg| configure_routes(cfg, state.clone())),
        )
        .await;

        // Send a request to an undefined endpoint.
        let req = test::TestRequest::get().uri("/undefined/path").to_request();
        let resp = test::call_service(&app, req).await;

        // Check that the response status is 404 Not Found.
        assert_eq!(resp.status(), StatusCode::NOT_FOUND);

        // Check the response body.
        let body = test::read_body(resp).await;
        let expected_body = "Endpoint '/undefined/path' not found";
        assert_eq!(body, expected_body);
    }
}
