//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
use std::sync::{Arc, RwLock};

use actix_web::{web, HttpResponse, Responder};
use serde::Serialize;

use crate::dev_server::{
    common::CheckTypeQuery,
    get_all_pkgs::get_all_pkgs,
    response::{ApiResponse, ErrorResponse, Status},
    DevServerState,
};
use ten_rust::pkg_info::pkg_type::PkgType;

#[derive(Serialize)]
struct CheckResponse {
    is_dirty: bool,
}

pub async fn check_manifest(
    state: web::Data<Arc<RwLock<DevServerState>>>,
    query: web::Query<CheckTypeQuery>,
) -> impl Responder {
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
            match query.0.check_type.as_str() {
                "dirty" => match app_pkg.is_manifest_equal_to_fs() {
                    Ok(is_dirty) => {
                        let response = ApiResponse {
                            status: Status::Ok,
                            data: CheckResponse { is_dirty },
                            meta: None,
                        };

                        HttpResponse::Ok().json(response)
                    }
                    Err(err) => {
                        let error_response = ErrorResponse {
                            status: Status::Fail,
                            message: format!(
                                "Failed to check manifest: {}",
                                err
                            ),
                            error: None,
                        };
                        HttpResponse::NotFound().json(error_response)
                    }
                },
                _ => HttpResponse::BadRequest().body("Invalid check type"),
            }
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
