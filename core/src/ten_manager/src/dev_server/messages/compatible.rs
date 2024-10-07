//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
use std::{
    str::FromStr,
    sync::{Arc, RwLock},
};

use actix_web::{web, HttpMessage, HttpRequest, HttpResponse, Responder};
use serde::{Deserialize, Serialize};

use crate::dev_server::{
    get_all_pkgs::get_all_pkgs,
    response::{ApiResponse, ErrorResponse, Status},
    DevServerState,
};
use ten_rust::pkg_info::{
    message::{MsgDirection, MsgType},
    predefined_graphs::extension::{
        get_compatible_cmd_extension, get_compatible_data_like_msg_extension,
        get_extension, get_extension_nodes_in_graph,
        get_pkg_info_for_extension, CompatibleExtensionAndMsg,
    },
};

#[derive(Debug, Deserialize, Serialize)]
pub struct InputData {
    pub app: String,
    pub graph: String,
    pub extension_group: String,
    pub extension: String,
    pub msg_type: String,
    pub msg_direction: String,
    pub msg_name: String,
}

#[derive(Debug, Deserialize, Serialize, PartialEq)]
pub struct DevServerCompatibleMsg {
    pub app: String,
    pub extension_group: String,
    pub extension: String,
    pub msg_type: MsgType,
    pub msg_direction: MsgDirection,
    pub msg_name: String,
}

impl From<CompatibleExtensionAndMsg<'_>> for DevServerCompatibleMsg {
    fn from(compatible: CompatibleExtensionAndMsg) -> Self {
        DevServerCompatibleMsg {
            app: compatible.extension.app.clone(),
            extension_group: compatible
                .extension
                .extension_group
                .clone()
                .unwrap()
                .clone(),
            extension: compatible.extension.name.clone(),
            msg_type: compatible.msg_type,
            msg_direction: compatible.msg_direction,
            msg_name: compatible.msg_name,
        }
    }
}

pub async fn get_compatible_messages(
    req: HttpRequest,
    state: web::Data<Arc<RwLock<DevServerState>>>,
    input: Result<web::Json<InputData>, actix_web::Error>,
) -> impl Responder {
    if req.content_type() != "application/json" {
        return HttpResponse::BadRequest().json(ErrorResponse {
            status: Status::Fail,
            message: "Content-Type must be application/json".to_string(),
            error: None,
        });
    }

    let input = match input {
        Ok(data) => data,
        Err(err) => {
            let error_response = ErrorResponse {
                status: Status::Fail,
                message: format!("Invalid input data format: {}", err),
                error: None,
            };
            return HttpResponse::BadRequest().json(error_response);
        }
    };

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

    if let Some(all_pkgs) = &state.all_pkgs {
        let extensions =
            match get_extension_nodes_in_graph(&input.graph, all_pkgs) {
                Ok(exts) => exts,
                Err(err) => {
                    let error_response = ErrorResponse {
                        status: Status::Fail,
                        message: format!(
                        "Error fetching runtime extensions for graph: {}: {}",
                        input.graph, err
                    ),
                        error: None,
                    };
                    return HttpResponse::NotFound().json(error_response);
                }
            };

        let extension = match get_extension(
            &extensions,
            &input.app,
            &input.extension_group,
            &input.extension,
        ) {
            Ok(ext) => ext,
            Err(err) => {
                let error_response = ErrorResponse {
                    status: Status::Fail,
                    message: format!(
                        "Failed to find the extension: {}: {}",
                        input.extension, err
                    ),
                    error: None,
                };
                return HttpResponse::NotFound().json(error_response);
            }
        };

        let msg_ty = match MsgType::from_str(&input.msg_type) {
            Ok(msg_ty) => msg_ty,
            Err(err) => {
                let error_response = ErrorResponse {
                    status: Status::Fail,
                    message: format!(
                        "Unsupported message type: {}: {}",
                        input.msg_type, err
                    ),
                    error: None,
                };
                return HttpResponse::InternalServerError()
                    .json(error_response);
            }
        };

        let msg_dir = match MsgDirection::from_str(&input.msg_direction) {
            Ok(msg_dir) => msg_dir,
            Err(err) => {
                let error_response = ErrorResponse {
                    status: Status::Fail,
                    message: format!(
                        "Unsupported message direction: {}: {}",
                        input.msg_direction, err
                    ),
                    error: None,
                };
                return HttpResponse::InternalServerError()
                    .json(error_response);
            }
        };

        let mut desired_msg_dir = msg_dir.clone();
        desired_msg_dir.toggle();

        let pkg_info = get_pkg_info_for_extension(extension, all_pkgs);
        if let Err(err) = pkg_info {
            let error_response = ErrorResponse {
                status: Status::Fail,
                message: format!(
                    "Error fetching runtime extensions for graph: {}: {}",
                    input.graph, err
                ),
                error: None,
            };
            return HttpResponse::NotFound().json(error_response);
        }

        let pkg_info = pkg_info.unwrap();

        let compatible_list = match msg_ty {
            MsgType::Cmd => {
                let src_cmd_schema =
                    pkg_info.schema_store.as_ref().and_then(|schema_store| {
                        match msg_dir {
                            MsgDirection::In => {
                                schema_store.cmd_in.get(input.msg_name.as_str())
                            }
                            MsgDirection::Out => schema_store
                                .cmd_out
                                .get(input.msg_name.as_str()),
                        }
                    });

                let results = match get_compatible_cmd_extension(
                    &extensions,
                    all_pkgs,
                    &desired_msg_dir,
                    src_cmd_schema,
                    input.msg_name.as_str(),
                ) {
                    Ok(results) => results,
                    Err(err) => {
                        let error_response = ErrorResponse {
                            status: Status::Fail,
                            message: format!(
                                "Failed to find compatible cmd/{}: {}",
                                input.msg_name, err
                            ),
                            error: None,
                        };
                        return HttpResponse::NotFound().json(error_response);
                    }
                };

                results
            }
            _ => {
                let src_msg_schema =
                    pkg_info.schema_store.as_ref().and_then(|schema_store| {
                        match msg_ty {
                            MsgType::Data => match msg_dir {
                                MsgDirection::In => schema_store
                                    .data_in
                                    .get(input.msg_name.as_str()),
                                MsgDirection::Out => schema_store
                                    .data_out
                                    .get(input.msg_name.as_str()),
                            },
                            MsgType::AudioFrame => match msg_dir {
                                MsgDirection::In => schema_store
                                    .audio_frame_in
                                    .get(input.msg_name.as_str()),
                                MsgDirection::Out => schema_store
                                    .audio_frame_out
                                    .get(input.msg_name.as_str()),
                            },
                            MsgType::VideoFrame => match msg_dir {
                                MsgDirection::In => schema_store
                                    .video_frame_in
                                    .get(input.msg_name.as_str()),
                                MsgDirection::Out => schema_store
                                    .video_frame_out
                                    .get(input.msg_name.as_str()),
                            },
                            _ => panic!("should not happen."),
                        }
                    });

                let results = match get_compatible_data_like_msg_extension(
                    &extensions,
                    all_pkgs,
                    &desired_msg_dir,
                    src_msg_schema,
                    &msg_ty,
                    input.msg_name.clone(),
                ) {
                    Ok(results) => results,
                    Err(err) => {
                        let error_response = ErrorResponse {
                            status: Status::Fail,
                            message: format!(
                                "Failed to find compatible cmd/{}: {}",
                                input.msg_name, err
                            ),
                            error: None,
                        };
                        return HttpResponse::NotFound().json(error_response);
                    }
                };

                results
            }
        };

        let results: Vec<DevServerCompatibleMsg> =
            compatible_list.into_iter().map(|v| v.into()).collect();

        let response = ApiResponse {
            status: Status::Ok,
            data: results,
            meta: None,
        };

        HttpResponse::Ok().json(response)
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
    use actix_web::{test, App};
    use serde_json::json;

    use super::*;
    use crate::{
        config::TmanConfig, dev_server::mock::tests::inject_all_pkgs_for_mock,
    };
    use ten_rust::pkg_info::default_app_loc;

    #[actix_web::test]
    async fn test_get_compatible_messages_success() {
        let mut dev_server_state = DevServerState {
            base_dir: None,
            all_pkgs: None,
            tman_config: TmanConfig::default(),
        };

        let all_pkgs_json = vec![
            (
                include_str!("test_data_embed/app_manifest.json").to_string(),
                include_str!("test_data_embed/app_property.json").to_string(),
            ),
            (
                include_str!("test_data_embed/extension_addon_1_manifest.json")
                    .to_string(),
                "{}".to_string(),
            ),
            (
                include_str!("test_data_embed/extension_addon_2_manifest.json")
                    .to_string(),
                "{}".to_string(),
            ),
        ];

        let inject_ret =
            inject_all_pkgs_for_mock(&mut dev_server_state, all_pkgs_json);
        assert!(inject_ret.is_ok());

        let dev_server_state = Arc::new(RwLock::new(dev_server_state));

        let app = test::init_service(
            App::new().app_data(web::Data::new(dev_server_state)).route(
                "/api/dev-server/v1/messages/compatible",
                web::post().to(get_compatible_messages),
            ),
        )
        .await;

        // Define input data.
        let input_data = json!({
          "app": "localhost",
          "graph": "default",
          "extension_group": "extension_group_1",
          "extension": "extension_1",
          "msg_type": "cmd",
          "msg_direction": "out",
          "msg_name": "test_cmd"
        });

        // Send request to the test server.
        let req = test::TestRequest::post()
            .uri("/api/dev-server/v1/messages/compatible")
            .set_json(&input_data)
            .to_request();

        // Call the service and get the response.
        let resp = test::call_service(&app, req).await;
        assert!(resp.status().is_success());

        let body = test::read_body(resp).await;
        let body_str = std::str::from_utf8(&body).unwrap();

        let compatibles: ApiResponse<Vec<DevServerCompatibleMsg>> =
            serde_json::from_str(body_str).unwrap();

        let expected_compatibles = vec![DevServerCompatibleMsg {
            app: default_app_loc(),
            extension_group: "extension_group_1".to_string(),
            extension: "extension_2".to_string(),
            msg_type: MsgType::Cmd,
            msg_direction: MsgDirection::In,
            msg_name: "test_cmd".to_string(),
        }];

        assert_eq!(compatibles.data, expected_compatibles);

        println!("Response body: {:?}", compatibles);
    }

    #[actix_web::test]
    async fn test_get_compatible_messages_fail() {
        let mut dev_server_state = DevServerState {
            base_dir: None,
            all_pkgs: None,
            tman_config: TmanConfig::default(),
        };

        let all_pkgs_json = vec![
            (
                include_str!("test_data_embed/app_manifest.json").to_string(),
                include_str!("test_data_embed/app_property.json").to_string(),
            ),
            (
                include_str!("test_data_embed/extension_addon_1_manifest.json")
                    .to_string(),
                "{}".to_string(),
            ),
            (
                include_str!("test_data_embed/extension_addon_2_manifest.json")
                    .to_string(),
                "{}".to_string(),
            ),
        ];

        let inject_ret =
            inject_all_pkgs_for_mock(&mut dev_server_state, all_pkgs_json);
        assert!(inject_ret.is_ok());

        let dev_server_state = Arc::new(RwLock::new(dev_server_state));

        let app = test::init_service(
            App::new().app_data(web::Data::new(dev_server_state)).route(
                "/api/dev-server/v1/messages/compatible",
                web::post().to(get_compatible_messages),
            ),
        )
        .await;

        // Define input data.
        let input_data = json!({
          "app": "localhost",
          "graph": "default",
          "extension_group": "default_extension_group",
          "extension": "default_extension_cpp",
          "msg_type": "data",
          "msg_direction": "in",
          "msg_name": "not_existing_cmd"
        });

        // Send request to the test server.
        let req = test::TestRequest::post()
            .uri("/api/dev-server/v1/messages/compatible")
            .set_json(&input_data)
            .to_request();

        // Call the service and get the response
        let resp = test::call_service(&app, req).await;

        assert!(resp.status().is_client_error());
    }

    #[actix_web::test]
    async fn test_get_compatible_messages_cmd_has_required_success() {
        let mut dev_server_state = DevServerState {
            base_dir: None,
            all_pkgs: None,
            tman_config: TmanConfig::default(),
        };

        let all_pkgs_json = vec![
            (
                include_str!("test_data_embed/app_manifest.json").to_string(),
                include_str!("test_data_embed/app_property.json").to_string(),
            ),
            (
                include_str!("test_data_embed/extension_addon_1_manifest.json")
                    .to_string(),
                "{}".to_string(),
            ),
            (
                include_str!("test_data_embed/extension_addon_2_manifest.json")
                    .to_string(),
                "{}".to_string(),
            ),
        ];

        let inject_ret =
            inject_all_pkgs_for_mock(&mut dev_server_state, all_pkgs_json);
        assert!(inject_ret.is_ok());

        let dev_server_state = Arc::new(RwLock::new(dev_server_state));

        let app = test::init_service(
            App::new().app_data(web::Data::new(dev_server_state)).route(
                "/api/dev-server/v1/messages/compatible",
                web::post().to(get_compatible_messages),
            ),
        )
        .await;

        // Define input data.
        let input_data = json!({
          "app": "localhost",
          "graph": "default",
          "extension_group": "extension_group_1",
          "extension": "extension_1",
          "msg_type": "cmd",
          "msg_direction": "out",
          "msg_name": "has_required"
        });

        // Send request to the test server.
        let req = test::TestRequest::post()
            .uri("/api/dev-server/v1/messages/compatible")
            .set_json(&input_data)
            .to_request();

        // Call the service and get the response.
        let resp = test::call_service(&app, req).await;
        assert!(resp.status().is_success());

        let body = test::read_body(resp).await;
        let body_str = std::str::from_utf8(&body).unwrap();

        let compatibles: ApiResponse<Vec<DevServerCompatibleMsg>> =
            serde_json::from_str(body_str).unwrap();

        let expected_compatibles = vec![DevServerCompatibleMsg {
            app: default_app_loc(),
            extension_group: "extension_group_1".to_string(),
            extension: "extension_2".to_string(),
            msg_type: MsgType::Cmd,
            msg_direction: MsgDirection::In,
            msg_name: "has_required".to_string(),
        }];

        assert_eq!(compatibles.data, expected_compatibles);

        println!("Response body: {:?}", compatibles);
    }

    #[actix_web::test]
    async fn test_get_compatible_messages_has_required_subset() {
        let mut dev_server_state = DevServerState {
            base_dir: None,
            all_pkgs: None,
            tman_config: TmanConfig::default(),
        };

        let all_pkgs_json = vec![
            (
                include_str!("test_data_embed/app_manifest.json").to_string(),
                include_str!("test_data_embed/app_property.json").to_string(),
            ),
            (
                include_str!("test_data_embed/extension_addon_1_manifest.json")
                    .to_string(),
                "{}".to_string(),
            ),
            (
                include_str!("test_data_embed/extension_addon_2_manifest.json")
                    .to_string(),
                "{}".to_string(),
            ),
        ];

        let inject_ret =
            inject_all_pkgs_for_mock(&mut dev_server_state, all_pkgs_json);
        assert!(inject_ret.is_ok());

        let dev_server_state = Arc::new(RwLock::new(dev_server_state));

        let app = test::init_service(
            App::new().app_data(web::Data::new(dev_server_state)).route(
                "/api/dev-server/v1/messages/compatible",
                web::post().to(get_compatible_messages),
            ),
        )
        .await;

        // Define input data.
        let input_data = json!({
          "app": "localhost",
          "graph": "default",
          "extension_group": "extension_group_1",
          "extension": "extension_1",
          "msg_type": "cmd",
          "msg_direction": "out",
          "msg_name": "has_required_mismatch"
        });

        // Send request to the test server.
        let req = test::TestRequest::post()
            .uri("/api/dev-server/v1/messages/compatible")
            .set_json(&input_data)
            .to_request();

        // Call the service and get the response.
        let resp = test::call_service(&app, req).await;

        assert!(resp.status().is_success());

        let body = test::read_body(resp).await;
        let body_str = std::str::from_utf8(&body).unwrap();

        let compatibles: ApiResponse<Vec<DevServerCompatibleMsg>> =
            serde_json::from_str(body_str).unwrap();

        let expected_compatibles = vec![DevServerCompatibleMsg {
            app: default_app_loc(),
            extension_group: "extension_group_1".to_string(),
            extension: "extension_2".to_string(),
            msg_type: MsgType::Cmd,
            msg_direction: MsgDirection::In,
            msg_name: "has_required_mismatch".to_string(),
        }];

        assert_eq!(compatibles.data, expected_compatibles);
    }

    #[actix_web::test]
    async fn test_get_compatible_messages_cmd_no_property() {
        let mut dev_server_state = DevServerState {
            base_dir: None,
            all_pkgs: None,
            tman_config: TmanConfig::default(),
        };

        // The first item is 'manifest.json', and the second item is
        // 'property.json'.
        let all_pkgs_json = vec![
            (
                include_str!("test_data_embed/app_manifest.json").to_string(),
                include_str!("test_data_embed/app_property.json").to_string(),
            ),
            (
                include_str!("test_data_embed/extension_addon_1_manifest.json")
                    .to_string(),
                "{}".to_string(),
            ),
            (
                include_str!("test_data_embed/extension_addon_2_manifest.json")
                    .to_string(),
                "{}".to_string(),
            ),
        ];

        let inject_ret =
            inject_all_pkgs_for_mock(&mut dev_server_state, all_pkgs_json);
        assert!(inject_ret.is_ok());

        let dev_server_state = Arc::new(RwLock::new(dev_server_state));

        let app = test::init_service(
            App::new().app_data(web::Data::new(dev_server_state)).route(
                "/api/dev-server/v1/messages/compatible",
                web::post().to(get_compatible_messages),
            ),
        )
        .await;

        // Define input data.
        let input_data = json!({
          "app": "localhost",
          "graph": "default",
          "extension_group": "extension_group_1",
          "extension": "extension_1",
          "msg_type": "cmd",
          "msg_direction": "out",
          "msg_name": "cmd1"
        });

        // Send request to the test server.
        let req = test::TestRequest::post()
            .uri("/api/dev-server/v1/messages/compatible")
            .set_json(&input_data)
            .to_request();

        // Call the service and get the response.
        let resp = test::call_service(&app, req).await;
        assert!(resp.status().is_success());

        let body = test::read_body(resp).await;
        let body_str = std::str::from_utf8(&body).unwrap();

        let compatibles: ApiResponse<Vec<DevServerCompatibleMsg>> =
            serde_json::from_str(body_str).unwrap();

        let expected_compatibles = vec![
            DevServerCompatibleMsg {
                app: default_app_loc(),
                extension_group: "extension_group_1".to_string(),
                extension: "extension_1".to_string(),
                msg_type: MsgType::Cmd,
                msg_direction: MsgDirection::In,
                msg_name: "cmd1".to_string(),
            },
            DevServerCompatibleMsg {
                app: default_app_loc(),
                extension_group: "extension_group_1".to_string(),
                extension: "extension_2".to_string(),
                msg_type: MsgType::Cmd,
                msg_direction: MsgDirection::In,
                msg_name: "cmd1".to_string(),
            },
        ];

        assert_eq!(compatibles.data, expected_compatibles);

        println!("Response body: {:?}", compatibles);
    }

    #[actix_web::test]
    async fn test_get_compatible_messages_cmd_property_overlap() {
        let mut dev_server_state = DevServerState {
            base_dir: None,
            all_pkgs: None,
            tman_config: TmanConfig::default(),
        };

        // The first item is 'manifest.json', and the second item is
        // 'property.json'.
        let all_pkgs_json = vec![
            (
                include_str!("test_data_embed/app_manifest.json").to_string(),
                include_str!("test_data_embed/app_property.json").to_string(),
            ),
            (
                include_str!("test_data_embed/extension_addon_1_manifest.json")
                    .to_string(),
                "{}".to_string(),
            ),
            (
                include_str!("test_data_embed/extension_addon_2_manifest.json")
                    .to_string(),
                "{}".to_string(),
            ),
        ];

        let inject_ret =
            inject_all_pkgs_for_mock(&mut dev_server_state, all_pkgs_json);
        assert!(inject_ret.is_ok());

        let dev_server_state = Arc::new(RwLock::new(dev_server_state));

        let app = test::init_service(
            App::new().app_data(web::Data::new(dev_server_state)).route(
                "/api/dev-server/v1/messages/compatible",
                web::post().to(get_compatible_messages),
            ),
        )
        .await;

        // Define input data.
        let input_data = json!({
          "app": "localhost",
          "graph": "default",
          "extension_group": "extension_group_1",
          "extension": "extension_1",
          "msg_type": "cmd",
          "msg_direction": "out",
          "msg_name": "cmd2"
        });

        // Send request to the test server.
        let req = test::TestRequest::post()
            .uri("/api/dev-server/v1/messages/compatible")
            .set_json(&input_data)
            .to_request();

        // Call the service and get the response.
        let resp = test::call_service(&app, req).await;
        assert!(resp.status().is_success());

        let body = test::read_body(resp).await;
        let body_str = std::str::from_utf8(&body).unwrap();

        let compatibles: ApiResponse<Vec<DevServerCompatibleMsg>> =
            serde_json::from_str(body_str).unwrap();

        let expected_compatibles = vec![DevServerCompatibleMsg {
            app: default_app_loc(),
            extension_group: "extension_group_1".to_string(),
            extension: "extension_2".to_string(),
            msg_type: MsgType::Cmd,
            msg_direction: MsgDirection::In,
            msg_name: "cmd2".to_string(),
        }];

        assert_eq!(compatibles.data, expected_compatibles);

        println!("Response body: {:?}", compatibles);
    }

    #[actix_web::test]
    async fn test_get_compatible_messages_cmd_property_required_missing() {
        let mut dev_server_state = DevServerState {
            base_dir: None,
            all_pkgs: None,
            tman_config: TmanConfig::default(),
        };

        // The first item is 'manifest.json', and the second item is
        // 'property.json'.
        let all_pkgs_json = vec![
            (
                include_str!("test_data_embed/app_manifest.json").to_string(),
                include_str!("test_data_embed/app_property.json").to_string(),
            ),
            (
                include_str!("test_data_embed/extension_addon_1_manifest.json")
                    .to_string(),
                "{}".to_string(),
            ),
            (
                include_str!("test_data_embed/extension_addon_2_manifest.json")
                    .to_string(),
                "{}".to_string(),
            ),
        ];

        let inject_ret =
            inject_all_pkgs_for_mock(&mut dev_server_state, all_pkgs_json);
        assert!(inject_ret.is_ok());

        let dev_server_state = Arc::new(RwLock::new(dev_server_state));

        let app = test::init_service(
            App::new().app_data(web::Data::new(dev_server_state)).route(
                "/api/dev-server/v1/messages/compatible",
                web::post().to(get_compatible_messages),
            ),
        )
        .await;

        // Define input data.
        let input_data = json!({
          "app": "localhost",
          "graph": "default",
          "extension_group": "extension_group_1",
          "extension": "extension_1",
          "msg_type": "cmd",
          "msg_direction": "out",
          "msg_name": "cmd3"
        });

        // Send request to the test server.
        let req = test::TestRequest::post()
            .uri("/api/dev-server/v1/messages/compatible")
            .set_json(&input_data)
            .to_request();

        // Call the service and get the response.
        let resp = test::call_service(&app, req).await;
        assert!(resp.status().is_success());

        let body = test::read_body(resp).await;
        let body_str = std::str::from_utf8(&body).unwrap();

        let compatibles: ApiResponse<Vec<DevServerCompatibleMsg>> =
            serde_json::from_str(body_str).unwrap();

        assert!(compatibles.data.is_empty());
    }

    #[actix_web::test]
    async fn test_get_compatible_messages_cmd_result() {
        let mut dev_server_state = DevServerState {
            base_dir: None,
            all_pkgs: None,
            tman_config: TmanConfig::default(),
        };

        // The first item is 'manifest.json', and the second item is
        // 'property.json'.
        let all_pkgs_json = vec![
            (
                include_str!("test_data_embed/app_manifest.json").to_string(),
                include_str!("test_data_embed/app_property.json").to_string(),
            ),
            (
                include_str!("test_data_embed/extension_addon_1_manifest.json")
                    .to_string(),
                "{}".to_string(),
            ),
            (
                include_str!("test_data_embed/extension_addon_2_manifest.json")
                    .to_string(),
                "{}".to_string(),
            ),
        ];

        let inject_ret =
            inject_all_pkgs_for_mock(&mut dev_server_state, all_pkgs_json);
        assert!(inject_ret.is_ok());

        let dev_server_state = Arc::new(RwLock::new(dev_server_state));

        let app = test::init_service(
            App::new().app_data(web::Data::new(dev_server_state)).route(
                "/api/dev-server/v1/messages/compatible",
                web::post().to(get_compatible_messages),
            ),
        )
        .await;

        // Define input data.
        let input_data = json!({
          "app": "localhost",
          "graph": "default",
          "extension_group": "extension_group_1",
          "extension": "extension_1",
          "msg_type": "cmd",
          "msg_direction": "out",
          "msg_name": "cmd4"
        });

        // Send request to the test server.
        let req = test::TestRequest::post()
            .uri("/api/dev-server/v1/messages/compatible")
            .set_json(&input_data)
            .to_request();

        // Call the service and get the response.
        let resp = test::call_service(&app, req).await;
        assert!(resp.status().is_success());

        let body = test::read_body(resp).await;
        let body_str = std::str::from_utf8(&body).unwrap();

        let compatibles: ApiResponse<Vec<DevServerCompatibleMsg>> =
            serde_json::from_str(body_str).unwrap();
        assert!(compatibles.data.is_empty());

        // let expected_compatibles = vec![DevServerCompatibleMsg {
        //     app: default_app_loc(),
        //     extension_group: "extension_group_1".to_string(),
        //     extension: "extension_2".to_string(),
        //     msg_type: MsgType::Cmd,
        //     msg_direction: MsgDirection::In,
        //     msg_name: "cmd4".to_string(),
        // }];
        //
        // assert_eq!(compatibles.data, expected_compatibles);
    }

    #[actix_web::test]
    async fn test_get_compatible_messages_data_no_property() {
        let mut dev_server_state = DevServerState {
            base_dir: None,
            all_pkgs: None,
            tman_config: TmanConfig::default(),
        };

        // The first item is 'manifest.json', and the second item is
        // 'property.json'.
        let all_pkgs_json = vec![
            (
                include_str!("test_data_embed/app_manifest.json").to_string(),
                include_str!("test_data_embed/app_property.json").to_string(),
            ),
            (
                include_str!("test_data_embed/extension_addon_1_manifest.json")
                    .to_string(),
                "{}".to_string(),
            ),
            (
                include_str!("test_data_embed/extension_addon_2_manifest.json")
                    .to_string(),
                "{}".to_string(),
            ),
        ];

        let inject_ret =
            inject_all_pkgs_for_mock(&mut dev_server_state, all_pkgs_json);
        assert!(inject_ret.is_ok());

        let dev_server_state = Arc::new(RwLock::new(dev_server_state));

        let app = test::init_service(
            App::new().app_data(web::Data::new(dev_server_state)).route(
                "/api/dev-server/v1/messages/compatible",
                web::post().to(get_compatible_messages),
            ),
        )
        .await;

        // Define input data.
        let input_data = json!({
          "app": "localhost",
          "graph": "default",
          "extension_group": "extension_group_1",
          "extension": "extension_1",
          "msg_type": "data",
          "msg_direction": "out",
          "msg_name": "data1"
        });

        // Send request to the test server.
        let req = test::TestRequest::post()
            .uri("/api/dev-server/v1/messages/compatible")
            .set_json(&input_data)
            .to_request();

        // Call the service and get the response.
        let resp = test::call_service(&app, req).await;
        assert!(resp.status().is_success());

        let body = test::read_body(resp).await;
        let body_str = std::str::from_utf8(&body).unwrap();

        let compatibles: ApiResponse<Vec<DevServerCompatibleMsg>> =
            serde_json::from_str(body_str).unwrap();
        assert!(compatibles.data.is_empty());

        // let expected_compatibles = vec![DevServerCompatibleMsg {
        //     app: default_app_loc(),
        //     extension_group: "extension_group_1".to_string(),
        //     extension: "extension_2".to_string(),
        //     msg_type: MsgType::Data,
        //     msg_direction: MsgDirection::In,
        //     msg_name: "data1".to_string(),
        // }];

        // assert_eq!(compatibles.data, expected_compatibles);
    }

    // TODO(Liu): Enable this when ManifestPropertyAttributes supports
    // properties/items.
    // #[actix_web::test]
    async fn test_get_compatible_messages_data_required_superset() {
        let mut dev_server_state = DevServerState {
            base_dir: None,
            all_pkgs: None,
            tman_config: TmanConfig::default(),
        };

        // The first item is 'manifest.json', and the second item is
        // 'property.json'.
        let all_pkgs_json = vec![
            (
                include_str!("test_data_embed/app_manifest.json").to_string(),
                include_str!("test_data_embed/app_property.json").to_string(),
            ),
            (
                include_str!("test_data_embed/extension_addon_1_manifest.json")
                    .to_string(),
                "{}".to_string(),
            ),
            (
                include_str!("test_data_embed/extension_addon_2_manifest.json")
                    .to_string(),
                "{}".to_string(),
            ),
        ];

        let inject_ret =
            inject_all_pkgs_for_mock(&mut dev_server_state, all_pkgs_json);
        assert!(inject_ret.is_ok());

        let dev_server_state = Arc::new(RwLock::new(dev_server_state));

        let app = test::init_service(
            App::new().app_data(web::Data::new(dev_server_state)).route(
                "/api/dev-server/v1/messages/compatible",
                web::post().to(get_compatible_messages),
            ),
        )
        .await;

        // Define input data.
        let input_data = json!({
          "app": "localhost",
          "graph": "default",
          "extension_group": "extension_group_1",
          "extension": "extension_1",
          "msg_type": "data",
          "msg_direction": "out",
          "msg_name": "data2"
        });

        // Send request to the test server.
        let req = test::TestRequest::post()
            .uri("/api/dev-server/v1/messages/compatible")
            .set_json(&input_data)
            .to_request();

        // Call the service and get the response.
        let resp = test::call_service(&app, req).await;
        assert!(resp.status().is_success());

        let body = test::read_body(resp).await;
        let body_str = std::str::from_utf8(&body).unwrap();

        let compatibles: ApiResponse<Vec<DevServerCompatibleMsg>> =
            serde_json::from_str(body_str).unwrap();

        assert!(compatibles.data.is_empty());
    }

    #[actix_web::test]
    async fn test_get_compatible_messages_video_target_has_property() {
        let mut dev_server_state = DevServerState {
            base_dir: None,
            all_pkgs: None,
            tman_config: TmanConfig::default(),
        };

        // The first item is 'manifest.json', and the second item is
        // 'property.json'.
        let all_pkgs_json = vec![
            (
                include_str!("test_data_embed/app_manifest.json").to_string(),
                include_str!("test_data_embed/app_property.json").to_string(),
            ),
            (
                include_str!("test_data_embed/extension_addon_1_manifest.json")
                    .to_string(),
                "{}".to_string(),
            ),
            (
                include_str!("test_data_embed/extension_addon_2_manifest.json")
                    .to_string(),
                "{}".to_string(),
            ),
        ];

        let inject_ret =
            inject_all_pkgs_for_mock(&mut dev_server_state, all_pkgs_json);
        assert!(inject_ret.is_ok());

        let dev_server_state = Arc::new(RwLock::new(dev_server_state));

        let app = test::init_service(
            App::new().app_data(web::Data::new(dev_server_state)).route(
                "/api/dev-server/v1/messages/compatible",
                web::post().to(get_compatible_messages),
            ),
        )
        .await;

        // Define input data.
        let input_data = json!({
          "app": "localhost",
          "graph": "default",
          "extension_group": "extension_group_1",
          "extension": "extension_1",
          "msg_type": "video_frame",
          "msg_direction": "out",
          "msg_name": "pcm_frame1"
        });

        // Send request to the test server.
        let req = test::TestRequest::post()
            .uri("/api/dev-server/v1/messages/compatible")
            .set_json(&input_data)
            .to_request();

        // Call the service and get the response.
        let resp = test::call_service(&app, req).await;
        assert!(resp.status().is_success());

        let body = test::read_body(resp).await;
        let body_str = std::str::from_utf8(&body).unwrap();

        let compatibles: ApiResponse<Vec<DevServerCompatibleMsg>> =
            serde_json::from_str(body_str).unwrap();

        let expected_compatibles = vec![DevServerCompatibleMsg {
            app: default_app_loc(),
            extension_group: "extension_group_1".to_string(),
            extension: "extension_1".to_string(),
            msg_type: MsgType::VideoFrame,
            msg_direction: MsgDirection::In,
            msg_name: "pcm_frame1".to_string(),
        }];

        assert_eq!(compatibles.data, expected_compatibles);
    }
}
