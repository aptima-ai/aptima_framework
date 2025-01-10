//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
mod addons;
mod base_dir;
mod common;
mod dir_list;
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

use axis_rust::pkg_info::PkgInfo;

use super::config::TmanConfig;
use terminal::ws_terminal;
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
    cfg.service(
        web::scope("/api/designer/v1")
            .app_data(state.clone())
            .route("/version", web::get().to(get_version))
            .route(
                "/addons/extensions",
                web::get().to(addons::extensions::get_extension_addons),
            )
            .route(
                "/addons/extensions/{name}",
                web::get().to(addons::extensions::get_extension_addon_by_name),
            )
            .route(
                "/packages/reload",
                web::post().to(packages::reload::clear_and_reload_pkgs),
            )
            .route("/graphs", web::get().to(graphs::get_graphs))
            .route(
                "/graphs/{graph_name}/nodes",
                web::get().to(graphs::nodes::get_graph_nodes),
            )
            .route(
                "/graphs/{graph_name}/connections",
                web::get().to(graphs::connections::get_graph_connections),
            )
            .route(
                "/graphs/{graph_name}",
                web::put().to(graphs::update::update_graph),
            )
            .route("/manifest", web::put().to(manifest::dump::dump_manifest))
            .route(
                "/manifest/check",
                web::get().to(manifest::check::check_manifest),
            )
            .route("/property", web::put().to(property::dump::dump_property))
            .route(
                "/property/check",
                web::get().to(property::check::check_property),
            )
            .route(
                "/messages/compatible",
                web::post().to(messages::compatible::get_compatible_messages),
            )
            .route(
                "/file-content/{path}",
                web::get().to(file_content::get_file_content),
            )
            .route(
                "/file-content/{path}",
                web::put().to(file_content::save_file_content),
            )
            .route("/base-dir", web::put().to(base_dir::set_base_dir))
            .route("/dir-list/{path}", web::get().to(dir_list::list_dir))
            .route("/ws/terminal", web::get().to(ws_terminal)),
    );
}
