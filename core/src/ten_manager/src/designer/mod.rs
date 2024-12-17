//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
mod addons;
mod common;
mod file_content;
pub mod frontend;
mod get_all_pkgs;
pub mod graphs;
mod manifest;
mod messages;
mod mock;
mod packages;
mod property;
pub mod response;
mod terminal;
mod version;

use std::sync::{Arc, RwLock};

use actix_web::web;

use ten_rust::pkg_info::PkgInfo;
use terminal::ws_terminal;

use super::config::TmanConfig;
use version::get_version;

pub struct DesignerState {
    pub base_dir: Option<String>,
    pub all_pkgs: Option<Vec<PkgInfo>>,
    pub tman_config: TmanConfig,
}

pub fn configure_routes(
    cfg: &mut web::ServiceConfig,
    state: web::Data<Arc<RwLock<DesignerState>>>,
) {
    cfg.app_data(state.clone())
        .route("/api/designer/v1/version", web::get().to(get_version))
        .route(
            "/api/designer/v1/addons/extensions",
            web::get().to(addons::extensions::get_extension_addons),
        )
        .route(
            "/api/designer/v1/addons/extensions/{name}",
            web::get().to(addons::extensions::get_extension_addon_by_name),
        )
        .route(
            "/api/designer/v1/packages/reload",
            web::post().to(packages::reload::clear_and_reload_pkgs),
        )
        .route("/api/designer/v1/graphs", web::get().to(graphs::get_graphs))
        .route(
            "/api/designer/v1/graphs/{graph_name}/nodes",
            web::get().to(graphs::nodes::get_graph_nodes),
        )
        .route(
            "/api/designer/v1/graphs/{graph_name}/connections",
            web::get().to(graphs::connections::get_graph_connections),
        )
        .route(
            "/api/designer/v1/graphs/{graph_name}",
            web::put().to(graphs::update::update_graph),
        )
        .route(
            "/api/designer/v1/manifest",
            web::put().to(manifest::dump::dump_manifest),
        )
        .route(
            "/api/designer/v1/manifest/check",
            web::get().to(manifest::check::check_manifest),
        )
        .route(
            "/api/designer/v1/property",
            web::put().to(property::dump::dump_property),
        )
        .route(
            "/api/designer/v1/property/check",
            web::get().to(property::check::check_property),
        )
        .route(
            "/api/designer/v1/messages/compatible",
            web::post().to(messages::compatible::get_compatible_messages),
        )
        .route(
            "/api/designer/v1/file-content/{path}",
            web::get().to(file_content::get_file_content),
        )
        .route(
            "/api/designer/v1/file-content/{path}",
            web::put().to(file_content::save_file_content),
        )
        .route("/ws/terminal", web::get().to(ws_terminal));
}
