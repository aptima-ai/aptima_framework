//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
use std::{
    fs,
    path::Path,
    sync::{Arc, RwLock},
};

use actix_web::{web, HttpResponse, Responder};
use serde::{Deserialize, Serialize};

use super::{
    response::{ApiResponse, Status},
    DesignerState,
};

#[derive(Serialize, Deserialize, Debug)]
pub struct FsEntry {
    pub name: String,
    pub path: String,
    pub is_dir: bool,
}

#[derive(Serialize, Deserialize, Debug)]
pub struct DirListResponse {
    pub entries: Vec<FsEntry>,
}

pub async fn list_dir(
    path: web::Path<String>,
    _state: web::Data<Arc<RwLock<DesignerState>>>,
) -> impl Responder {
    let path_str = path.into_inner();
    let path_obj = Path::new(&path_str);

    // If the path does not exist, return 404.
    if !path_obj.exists() {
        let response = ApiResponse {
            status: Status::Fail,
            data: (),
            meta: None,
        };
        return HttpResponse::NotFound().json(response);
    }

    // Collect the returned results.
    let mut entries = Vec::new();

    if path_obj.is_file() {
        // If it is a file, return only the file information.
        if let Some(file_name) = path_obj.file_name() {
            entries.push(FsEntry {
                name: file_name.to_string_lossy().to_string(),
                path: path_str.clone(),
                is_dir: false,
            });
        }
    } else if path_obj.is_dir() {
        // If it is a folder, list its immediate items.
        if let Ok(dir_entries) = fs::read_dir(path_obj) {
            for entry in dir_entries.flatten() {
                let file_type = entry.file_type();
                if let Ok(ft) = file_type {
                    let file_name =
                        entry.file_name().to_string_lossy().to_string();
                    let full_path = entry.path().to_string_lossy().to_string();
                    entries.push(FsEntry {
                        name: file_name,
                        path: full_path,
                        is_dir: ft.is_dir(),
                    });
                }
            }
        }
    }

    let response = ApiResponse {
        status: Status::Ok,
        data: DirListResponse { entries },
        meta: None,
    };
    HttpResponse::Ok().json(response)
}

#[cfg(test)]
mod tests {
    use crate::config::TmanConfig;

    use super::*;
    use actix_web::{test, App};
    use std::fs::{self, File};
    use std::io::Write;
    use tempfile::tempdir;
    use urlencoding::encode;

    #[actix_web::test]
    async fn test_list_dir_with_file_path() {
        // Create a temporary directory.
        let dir = tempdir().unwrap();
        let file_path = dir.path().join("test_file.txt");
        let mut file = File::create(&file_path).unwrap();
        writeln!(file, "Hello, world!").unwrap();

        // Initialize DesignerState.
        let state = web::Data::new(Arc::new(RwLock::new(DesignerState {
            base_dir: None,
            all_pkgs: None,
            tman_config: TmanConfig::default(),
        })));

        // Configure the `list_dir` route.
        let app = test::init_service(App::new().app_data(state.clone()).route(
            "/api/designer/v1/dir-list/{path}",
            web::get().to(list_dir),
        ))
        .await;

        // Construct the request.
        let req_path = file_path.to_string_lossy().to_string();
        let encoded_path = encode(&req_path);
        let req = test::TestRequest::get()
            .uri(&format!("/api/designer/v1/dir-list/{}", encoded_path))
            .to_request();
        let resp = test::call_service(&app, req).await;

        assert!(resp.status().is_success());

        let body = test::read_body(resp).await;
        let response: ApiResponse<DirListResponse> =
            serde_json::from_slice(&body).unwrap();

        assert_eq!(response.status, Status::Ok);
        assert_eq!(response.data.entries.len(), 1);
        let entry = &response.data.entries[0];
        assert_eq!(entry.name, "test_file.txt");
        assert_eq!(entry.path, req_path);
        assert!(!entry.is_dir);
    }

    #[actix_web::test]
    async fn test_list_dir_with_directory_path() {
        // Create a temporary directory.
        let dir = tempdir().unwrap();
        let sub_dir = dir.path().join("sub_dir");
        fs::create_dir(&sub_dir).unwrap();
        let file1 = dir.path().join("file1.txt");
        let file2 = sub_dir.join("file2.txt");
        let mut f1 = File::create(&file1).unwrap();
        writeln!(f1, "File 1").unwrap();
        let mut f2 = File::create(&file2).unwrap();
        writeln!(f2, "File 2").unwrap();

        // Initialize DesignerState.
        let state = web::Data::new(Arc::new(RwLock::new(DesignerState {
            base_dir: None,
            all_pkgs: None,
            tman_config: TmanConfig::default(),
        })));

        // Configure the `list_dir` route.
        let app = test::init_service(App::new().app_data(state.clone()).route(
            "/api/designer/v1/dir-list/{path}",
            web::get().to(list_dir),
        ))
        .await;

        // Construct the request.
        let req_path = dir.path().to_string_lossy().to_string();
        let encoded_path = encode(&req_path);
        let req = test::TestRequest::get()
            .uri(&format!("/api/designer/v1/dir-list/{}", encoded_path))
            .to_request();
        let resp = test::call_service(&app, req).await;

        assert!(resp.status().is_success());

        let body = test::read_body(resp).await;
        let response: ApiResponse<DirListResponse> =
            serde_json::from_slice(&body).unwrap();

        assert_eq!(response.status, Status::Ok);
        assert_eq!(response.data.entries.len(), 2);

        let mut entries = response.data.entries.iter().collect::<Vec<_>>();
        entries.sort_by_key(|e| &e.name);
        assert_eq!(entries[0].name, "file1.txt");
        assert_eq!(entries[0].path, file1.to_string_lossy());
        assert!(!entries[0].is_dir);
        assert_eq!(entries[1].name, "sub_dir");
        assert_eq!(entries[1].path, sub_dir.to_string_lossy());
        assert!(entries[1].is_dir);
    }

    #[actix_web::test]
    async fn test_list_dir_with_non_existing_path() {
        let state = web::Data::new(Arc::new(RwLock::new(DesignerState {
            base_dir: None,
            all_pkgs: None,
            tman_config: TmanConfig::default(),
        })));

        let app = test::init_service(App::new().app_data(state.clone()).route(
            "/api/designer/v1/dir-list/{path}",
            web::get().to(list_dir),
        ))
        .await;

        // Construct an invalid path.
        let non_existing_path = "/path/to/nonexistent".to_string();
        let encoded_path = encode(&non_existing_path);
        let req = test::TestRequest::get()
            .uri(&format!("/api/designer/v1/dir-list/{}", encoded_path))
            .to_request();
        let resp = test::call_service(&app, req).await;

        assert_eq!(resp.status(), actix_web::http::StatusCode::NOT_FOUND);

        let body = test::read_body(resp).await;
        let response: ApiResponse<()> = serde_json::from_slice(&body).unwrap();

        assert_eq!(response.status, Status::Fail);
    }
}
