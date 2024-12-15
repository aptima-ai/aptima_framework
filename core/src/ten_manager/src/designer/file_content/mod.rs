//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
use std::fs;
use std::sync::{Arc, RwLock};

use actix_web::{web, HttpResponse, Responder};
use serde::{Deserialize, Serialize};

use super::{
    response::{ApiResponse, Status},
    DesignerState,
};

#[derive(Serialize, Deserialize, Debug)]
struct FileContentResponse {
    content: String,
}

pub async fn get_file_content(
    path: web::Path<String>,
    _state: web::Data<Arc<RwLock<DesignerState>>>,
) -> impl Responder {
    let file_path = path.into_inner();

    match fs::read_to_string(&file_path) {
        Ok(content) => {
            let response = ApiResponse {
                status: Status::Ok,
                data: FileContentResponse { content },
                meta: None,
            };
            HttpResponse::Ok().json(response)
        }
        Err(err) => {
            eprintln!("Error reading file at path {}: {}", file_path, err);
            let response = ApiResponse {
                status: Status::Fail,
                data: (),
                meta: None,
            };
            HttpResponse::BadRequest().json(response)
        }
    }
}
