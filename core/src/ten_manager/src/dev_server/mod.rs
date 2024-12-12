//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
mod addons;
mod common;
// TODO(Wei): Enable this.
// pub mod frontend;
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

use actix_web::web;

use ten_rust::pkg_info::PkgInfo;

use super::config::TmanConfig;
use version::get_version;

pub struct DevServerState {
    pub base_dir: Option<String>,
    pub all_pkgs: Option<Vec<PkgInfo>>,
    pub tman_config: TmanConfig,
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
        );
}
