//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
use std::sync::{Arc, RwLock};

use actix_web::{web, HttpResponse, Responder};
use serde::{Deserialize, Serialize};

use ten_rust::pkg_info::pkg_type::PkgType;

use crate::dev_server::{
    common::{
        get_dev_server_api_cmd_likes_from_pkg,
        get_dev_server_api_data_likes_from_pkg,
        get_dev_server_property_hashmap_from_pkg,
    },
    get_all_pkgs::get_all_pkgs,
    graphs::nodes::DevServerApi,
    response::{ApiResponse, ErrorResponse, Status},
    DevServerState,
};

#[derive(Serialize, Deserialize, Debug, PartialEq)]
struct DevServerExtensionAddon {
    #[serde(rename = "name")]
    addon_name: String,

    #[serde(skip_serializing_if = "Option::is_none")]
    pub api: Option<DevServerApi>,
}

pub async fn get_extension_addons(
    state: web::Data<Arc<RwLock<DevServerState>>>,
) -> impl Responder {
    let mut state = state.write().unwrap();

    // Fetch all packages if not already done.
    if let Err(err) = get_all_pkgs(&mut state) {
        let error_response =
            ErrorResponse::from_error(&err, "Error fetching packages:");
        return HttpResponse::NotFound().json(error_response);
    }

    if let Some(all_pkgs) = &state.all_pkgs {
        let mut extensions = Vec::new();

        // Find all the packages with type "extension".
        for pkg_info_with_src in all_pkgs {
            if pkg_info_with_src.pkg_identity.pkg_type == PkgType::Extension {
                extensions.push(DevServerExtensionAddon {
                    addon_name: pkg_info_with_src.pkg_identity.name.clone(),
                    api: pkg_info_with_src.api.as_ref().map(|api| {
                        DevServerApi {
                            property: if api.property.is_empty() {
                                None
                            } else {
                                Some(get_dev_server_property_hashmap_from_pkg(
                                    api.property.clone(),
                                ))
                            },

                            cmd_in: if api.cmd_in.is_empty() {
                                None
                            } else {
                                Some(get_dev_server_api_cmd_likes_from_pkg(
                                    api.cmd_in.clone(),
                                ))
                            },
                            cmd_out: if api.cmd_out.is_empty() {
                                None
                            } else {
                                Some(get_dev_server_api_cmd_likes_from_pkg(
                                    api.cmd_out.clone(),
                                ))
                            },

                            data_in: if api.data_in.is_empty() {
                                None
                            } else {
                                Some(get_dev_server_api_data_likes_from_pkg(
                                    api.data_in.clone(),
                                ))
                            },
                            data_out: if api.data_out.is_empty() {
                                None
                            } else {
                                Some(get_dev_server_api_data_likes_from_pkg(
                                    api.data_out.clone(),
                                ))
                            },

                            audio_frame_in: if api.audio_frame_in.is_empty() {
                                None
                            } else {
                                Some(get_dev_server_api_data_likes_from_pkg(
                                    api.audio_frame_in.clone(),
                                ))
                            },
                            audio_frame_out: if api.audio_frame_out.is_empty() {
                                None
                            } else {
                                Some(get_dev_server_api_data_likes_from_pkg(
                                    api.audio_frame_out.clone(),
                                ))
                            },

                            video_frame_in: if api.video_frame_in.is_empty() {
                                None
                            } else {
                                Some(get_dev_server_api_data_likes_from_pkg(
                                    api.video_frame_in.clone(),
                                ))
                            },
                            video_frame_out: if api.video_frame_out.is_empty() {
                                None
                            } else {
                                Some(get_dev_server_api_data_likes_from_pkg(
                                    api.video_frame_out.clone(),
                                ))
                            },
                        }
                    }),
                });
            }
        }

        let response = ApiResponse {
            status: Status::Ok,
            data: extensions,
            meta: None,
        };

        HttpResponse::Ok().json(response)
    } else {
        let error_response = ErrorResponse {
            status: Status::Fail,
            message: "Base directory or package information is not set"
                .to_string(),
            error: None,
        };
        HttpResponse::BadRequest().json(error_response)
    }
}

#[cfg(test)]
mod tests {
    use crate::{
        config::TmanConfig,
        dev_server::{
            graphs::nodes::{
                DevServerApiCmdLike, DevServerApiDataLike,
                DevServerPropertyAttributes, DevServerPropertyItem,
            },
            mock::tests::inject_all_pkgs_for_mock,
        },
    };

    use super::*;
    use actix_web::{test, App};

    #[actix_web::test]
    async fn test_get_extension_addons() {
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
            (
                include_str!("test_data_embed/extension_addon_3_manifest.json")
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
                "/api/dev-server/v1/addons/extensions",
                web::get().to(get_extension_addons),
            ),
        )
        .await;

        let req = test::TestRequest::get()
            .uri("/api/dev-server/v1/addons/extensions")
            .to_request();
        let resp = test::call_service(&app, req).await;

        assert!(resp.status().is_success());

        let body = test::read_body(resp).await;
        let body_str = std::str::from_utf8(&body).unwrap();

        let addons: ApiResponse<Vec<DevServerExtensionAddon>> =
            serde_json::from_str(body_str).unwrap();

        let expected_addons = vec![
            DevServerExtensionAddon {
                addon_name: "extension_addon_1".to_string(),
                api: Some(DevServerApi {
                    property: None,
                    cmd_in: None,
                    cmd_out: Some(vec![
                        DevServerApiCmdLike {
                            name: "test_cmd".to_string(),
                            property: Some(vec![DevServerPropertyItem {
                                name: "test_property".to_string(),
                                attributes: DevServerPropertyAttributes {
                                    prop_type: "int8".to_string(),
                                },
                            }]),
                            required: None,
                            result: None,
                        },
                        DevServerApiCmdLike {
                            name: "has_required".to_string(),
                            property: Some(vec![DevServerPropertyItem {
                                name: "foo".to_string(),
                                attributes: DevServerPropertyAttributes {
                                    prop_type: "string".to_string(),
                                },
                            }]),
                            required: Some(vec!["foo".to_string()]),
                            result: None,
                        },
                        DevServerApiCmdLike {
                            name: "has_required_mismatch".to_string(),
                            property: Some(vec![DevServerPropertyItem {
                                name: "foo".to_string(),
                                attributes: DevServerPropertyAttributes {
                                    prop_type: "string".to_string(),
                                },
                            }]),
                            required: Some(vec!["foo".to_string()]),
                            result: None,
                        },
                    ]),
                    data_in: None,
                    data_out: None,
                    audio_frame_in: None,
                    audio_frame_out: None,
                    video_frame_in: None,
                    video_frame_out: None,
                }),
            },
            DevServerExtensionAddon {
                addon_name: "extension_addon_2".to_string(),
                api: Some(DevServerApi {
                    property: None,
                    cmd_in: Some(vec![
                        DevServerApiCmdLike {
                            name: "test_cmd".to_string(),
                            property: Some(vec![DevServerPropertyItem {
                                name: "test_property".to_string(),
                                attributes: DevServerPropertyAttributes {
                                    prop_type: "int32".to_string(),
                                },
                            }]),
                            required: None,
                            result: None,
                        },
                        DevServerApiCmdLike {
                            name: "another_test_cmd".to_string(),
                            property: Some(vec![DevServerPropertyItem {
                                name: "test_property".to_string(),
                                attributes: DevServerPropertyAttributes {
                                    prop_type: "int8".to_string(),
                                },
                            }]),
                            required: None,
                            result: None,
                        },
                        DevServerApiCmdLike {
                            name: "has_required".to_string(),
                            property: Some(vec![DevServerPropertyItem {
                                name: "foo".to_string(),
                                attributes: DevServerPropertyAttributes {
                                    prop_type: "string".to_string(),
                                },
                            }]),
                            required: Some(vec!["foo".to_string()]),
                            result: None,
                        },
                        DevServerApiCmdLike {
                            name: "has_required_mismatch".to_string(),
                            property: Some(vec![DevServerPropertyItem {
                                name: "foo".to_string(),
                                attributes: DevServerPropertyAttributes {
                                    prop_type: "string".to_string(),
                                },
                            }]),
                            required: None,
                            result: None,
                        },
                    ]),
                    cmd_out: None,
                    data_in: None,
                    data_out: Some(vec![DevServerApiDataLike {
                        name: "data_has_required".to_string(),
                        property: Some(vec![DevServerPropertyItem {
                            name: "foo".to_string(),
                            attributes: DevServerPropertyAttributes {
                                prop_type: "int8".to_string(),
                            },
                        }]),
                        required: Some(vec!["foo".to_string()]),
                    }]),
                    audio_frame_in: None,
                    audio_frame_out: None,
                    video_frame_in: None,
                    video_frame_out: None,
                }),
            },
            DevServerExtensionAddon {
                addon_name: "extension_addon_3".to_string(),
                api: Some(DevServerApi {
                    property: None,
                    cmd_in: Some(vec![DevServerApiCmdLike {
                        name: "test_cmd".to_string(),
                        property: Some(vec![DevServerPropertyItem {
                            name: "test_property".to_string(),
                            attributes: DevServerPropertyAttributes {
                                prop_type: "string".to_string(),
                            },
                        }]),
                        required: None,
                        result: None,
                    }]),
                    cmd_out: None,
                    data_in: Some(vec![DevServerApiDataLike {
                        name: "data_has_required".to_string(),
                        property: Some(vec![DevServerPropertyItem {
                            name: "foo".to_string(),
                            attributes: DevServerPropertyAttributes {
                                prop_type: "int8".to_string(),
                            },
                        }]),
                        required: Some(vec!["foo".to_string()]),
                    }]),
                    data_out: None,
                    audio_frame_in: None,
                    audio_frame_out: None,
                    video_frame_in: None,
                    video_frame_out: None,
                }),
            },
        ];

        assert_eq!(addons.data, expected_addons);

        let json: ApiResponse<Vec<DevServerExtensionAddon>> =
            serde_json::from_str(body_str).unwrap();
        let pretty_json = serde_json::to_string_pretty(&json).unwrap();
        println!("Response body: {}", pretty_json);
    }
}
