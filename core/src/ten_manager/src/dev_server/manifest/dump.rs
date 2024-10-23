//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
use std::{
    path::Path,
    sync::{Arc, RwLock},
};

use actix_web::{web, HttpResponse, Responder};
use serde::Serialize;

use crate::{
    constants::MANIFEST_JSON_FILENAME,
    dev_server::{
        get_all_pkgs::get_all_pkgs,
        response::{ApiResponse, ErrorResponse, Status},
        DevServerState,
    },
};
use ten_rust::pkg_info::manifest::dump_manifest_str_to_file;
use ten_rust::pkg_info::pkg_type::PkgType;

#[derive(Serialize, Debug, PartialEq)]
struct DumpResponse {
    success: bool,
}

pub async fn dump_manifest(
    state: web::Data<Arc<RwLock<DevServerState>>>,
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
            let new_manifest_str =
                app_pkg.manifest.clone().unwrap().to_string();

            let manifest_file_path = Path::join(
                Path::new(state.base_dir.as_ref().unwrap()),
                MANIFEST_JSON_FILENAME,
            );

            if let Err(err) =
                dump_manifest_str_to_file(&new_manifest_str, manifest_file_path)
            {
                let error_response = ErrorResponse::from_error(
                    &err,
                    "Failed to dump new manifest content to manifest file:",
                );
                return HttpResponse::NotFound().json(error_response);
            }

            let response = ApiResponse {
                status: Status::Ok,
                data: DumpResponse { success: true },
                meta: None,
            };

            HttpResponse::Ok().json(response)
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
