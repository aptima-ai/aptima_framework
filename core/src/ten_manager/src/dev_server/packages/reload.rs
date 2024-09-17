//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
use std::sync::{Arc, RwLock};

use actix_web::{web, HttpResponse, Responder};

use crate::dev_server::{
    get_all_pkgs::get_all_pkgs,
    response::{ApiResponse, ErrorResponse, Status},
    DevServerState,
};

pub async fn clear_and_reload_pkgs(
    state: web::Data<Arc<RwLock<DevServerState>>>,
) -> impl Responder {
    let mut state = state.write().unwrap();

    // Clear the existing packages
    state.all_pkgs = None;

    // Attempt to reload the packages
    match get_all_pkgs(&mut state) {
        Ok(_) => HttpResponse::Ok().json(ApiResponse {
            status: Status::Ok,
            data: "Packages reloaded successfully",
            meta: None,
        }),
        Err(err) => HttpResponse::InternalServerError().json(ErrorResponse {
            status: Status::Fail,
            message: format!("Failed to reload packages: {}", err),
            error: None,
        }),
    }
}
