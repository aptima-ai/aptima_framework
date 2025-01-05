//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
use std::{
    path::Path,
    sync::{Arc, RwLock},
};

use actix_web::{web, HttpResponse, Responder};
use serde::{Deserialize, Serialize};

use super::{response::ErrorResponse, DesignerState};
use crate::{
    designer::response::{ApiResponse, Status},
    fs::check_is_app_folder,
};

#[derive(Deserialize, Serialize, Debug)]
pub struct SetBaseDirRequest {
    pub base_dir: String,
}

#[derive(Serialize, Deserialize, Debug)]
pub struct SetBaseDirResponse {
    pub success: bool,
}

pub async fn set_base_dir(
    req: web::Json<SetBaseDirRequest>,
    state: web::Data<Arc<RwLock<DesignerState>>>,
) -> impl Responder {
    let mut state = state.write().unwrap();

    match check_is_app_folder(Path::new(&req.base_dir)) {
        Ok(_) => {
            // Update base_dir.
            state.base_dir = Some(req.base_dir.clone());

            // Reset the all_pkgs, so that it will be reloaded when needed.
            state.all_pkgs = None;

            let response = ApiResponse {
                status: Status::Ok,
                data: serde_json::json!({ "success": true }),
                meta: None,
            };

            HttpResponse::Ok().json(response)
        }
        Err(err) => {
            let error_response = ErrorResponse::from_error(
                &err,
                format!("{} is not an app folder: ", &req.base_dir).as_str(),
            );
            HttpResponse::NotFound().json(error_response)
        }
    }
}

#[cfg(test)]
mod tests {
    use actix_web::{test, App};

    use super::*;
    use crate::config::TmanConfig;

    #[actix_web::test]
    async fn test_set_base_dir_success() {
        let designer_state = DesignerState {
            base_dir: Some("/initial/path".to_string()),
            all_pkgs: Some(vec![]),
            tman_config: TmanConfig::default(),
        };
        let designer_state = Arc::new(RwLock::new(designer_state));

        let app = test::init_service(
            App::new()
                .app_data(web::Data::new(designer_state.clone()))
                .route(
                    "/api/designer/v1/base-dir",
                    web::put().to(set_base_dir),
                ),
        )
        .await;

        let new_base_dir = SetBaseDirRequest {
            base_dir: "/not/a/correct/app/folder/path".to_string(),
        };

        let req = test::TestRequest::put()
            .uri("/api/designer/v1/base-dir")
            .set_json(&new_base_dir)
            .to_request();
        let resp: Result<
            ApiResponse<SetBaseDirResponse>,
            std::boxed::Box<dyn std::error::Error>,
        > = test::try_call_and_read_body_json(&app, req).await;

        assert!(resp.is_err());
    }
}
