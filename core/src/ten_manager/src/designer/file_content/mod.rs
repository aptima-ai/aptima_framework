//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
use std::fs;
use std::path::Path;
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

#[derive(Serialize, Deserialize, Debug)]
pub struct SaveFileRequest {
    content: String,
}

pub async fn save_file_content(
    path: web::Path<String>,
    req: web::Json<SaveFileRequest>,
    _state: web::Data<Arc<RwLock<DesignerState>>>,
) -> HttpResponse {
    let file_path_str = path.into_inner();
    let content = &req.content; // Access the content field.

    let file_path = Path::new(&file_path_str);

    // Attempt to create parent directories if they don't exist.
    if let Some(parent) = file_path.parent() {
        if let Err(e) = fs::create_dir_all(parent) {
            eprintln!(
                "Error creating directories for {}: {}",
                parent.display(),
                e
            );
            let response = ApiResponse {
                status: Status::Fail,
                data: (),
                meta: None,
            };
            return HttpResponse::BadRequest().json(response);
        }
    }

    match fs::write(file_path, content) {
        Ok(_) => {
            let response = ApiResponse {
                status: Status::Ok,
                data: (),
                meta: None,
            };
            HttpResponse::Ok().json(response)
        }
        Err(err) => {
            eprintln!(
                "Error writing file at path {}: {}",
                file_path.display(),
                err
            );
            let response = ApiResponse {
                status: Status::Fail,
                data: (),
                meta: None,
            };
            HttpResponse::BadRequest().json(response)
        }
    }
}
