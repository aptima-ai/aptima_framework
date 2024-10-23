//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
use std::sync::{Arc, RwLock};

use actix_web::{web, HttpResponse, Responder};
use serde::{Deserialize, Serialize};

use crate::dev_server::{
    common::CheckTypeQuery,
    get_all_pkgs::get_all_pkgs,
    response::{ApiResponse, ErrorResponse, Status},
    DevServerState,
};
use ten_rust::pkg_info::pkg_type::PkgType;

#[derive(Serialize, Deserialize)]
struct CheckResponse {
    is_dirty: bool,
}

pub async fn check_property(
    state: web::Data<Arc<RwLock<DevServerState>>>,
    query: web::Query<CheckTypeQuery>,
) -> impl Responder {
    let mut state = state.write().unwrap();

    // Fetch all packages if not already done.
    if let Err(err) = get_all_pkgs(&mut state) {
        let error_response =
            ErrorResponse::from_error(&err, "Error fetching packages:");
        return HttpResponse::NotFound().json(error_response);
    }

    if let Some(pkgs) = &mut state.all_pkgs {
        if let Some(app_pkg) = pkgs
            .iter_mut()
            .find(|pkg| pkg.pkg_identity.pkg_type == PkgType::App)
        {
            match query.0.check_type.as_str() {
                "dirty" => match app_pkg.is_property_equal_to_fs() {
                    Ok(is_dirty) => {
                        let response = ApiResponse {
                            status: Status::Ok,
                            data: CheckResponse { is_dirty },
                            meta: None,
                        };

                        HttpResponse::Ok().json(response)
                    }
                    Err(err) => {
                        let error_response = ErrorResponse::from_error(
                            &err,
                            "Failed to check property:",
                        );
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

#[cfg(test)]
mod tests {
    use super::*;
    use crate::{
        config::TmanConfig, dev_server::mock::tests::inject_all_pkgs_for_mock,
    };
    use actix_web::{test, App};
    use std::vec;

    #[actix_web::test]
    async fn test_check_property_is_not_dirty() {
        let mut dev_server_state = DevServerState {
            base_dir: None,
            all_pkgs: None,
            tman_config: TmanConfig::default(),
        };

        // The first item is 'manifest.json', and the second item is
        // 'property.json'.
        let all_pkgs_json = vec![(
            include_str!("test_data_embed/app_manifest.json").to_string(),
            include_str!("test_data_embed/app_property.json").to_string(),
        )];

        let inject_ret =
            inject_all_pkgs_for_mock(&mut dev_server_state, all_pkgs_json);
        assert!(inject_ret.is_ok());

        let dev_server_state = Arc::new(RwLock::new(dev_server_state));
        let app = test::init_service(
            App::new().app_data(web::Data::new(dev_server_state)).route(
                "/api/dev-server/v1/property/check",
                web::get().to(check_property),
            ),
        )
        .await;

        let req = test::TestRequest::get()
            .uri("/api/dev-server/v1/property/check?type=dirty")
            .to_request();
        let resp = test::call_service(&app, req).await;

        assert!(resp.status().is_success());

        let body = test::read_body(resp).await;
        let body_str = std::str::from_utf8(&body).unwrap();

        let check_response: ApiResponse<CheckResponse> =
            serde_json::from_str(body_str).unwrap();

        assert!(!check_response.data.is_dirty);
    }
}
