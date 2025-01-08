//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
use std::sync::{Arc, RwLock};

use actix_web::{web, HttpResponse, Responder};
use serde::{Deserialize, Serialize};

use axis_rust::pkg_info::{pkg_type::PkgType, PkgInfo};

use crate::designer::{
    common::{
        get_designer_api_cmd_likes_from_pkg,
        get_designer_api_data_likes_from_pkg,
        get_designer_property_hashmap_from_pkg,
    },
    get_all_pkgs::get_all_pkgs,
    graphs::nodes::DesignerApi,
    response::{ApiResponse, ErrorResponse, Status},
    DesignerState,
};

#[derive(Serialize, Deserialize, Debug, PartialEq)]
struct DesignerExtensionAddon {
    #[serde(rename = "name")]
    addon_name: String,

    url: String,

    #[serde(skip_serializing_if = "Option::is_none")]
    pub api: Option<DesignerApi>,
}

fn retrieve_extension_addons(
    state: &mut DesignerState,
) -> Result<Vec<DesignerExtensionAddon>, ErrorResponse> {
    if let Err(err) = get_all_pkgs(state) {
        return Err(ErrorResponse::from_error(
            &err,
            "Error fetching packages:",
        ));
    }

    if let Some(all_pkgs) = &state.all_pkgs {
        let extensions = all_pkgs
            .iter()
            .filter(|pkg| {
                pkg.basic_info.type_and_name.pkg_type == PkgType::Extension
            })
            .map(|pkg_info_with_src| {
                map_pkg_to_extension_addon(pkg_info_with_src)
            })
            .collect();

        Ok(extensions)
    } else {
        Err(ErrorResponse {
            status: Status::Fail,
            message: "Base directory or package information is not set"
                .to_string(),
            error: None,
        })
    }
}

fn map_pkg_to_extension_addon(
    pkg_info_with_src: &PkgInfo,
) -> DesignerExtensionAddon {
    DesignerExtensionAddon {
        addon_name: pkg_info_with_src.basic_info.type_and_name.name.clone(),
        url: pkg_info_with_src.url.clone(),
        api: pkg_info_with_src.api.as_ref().map(|api| DesignerApi {
            property: if api.property.is_empty() {
                None
            } else {
                Some(get_designer_property_hashmap_from_pkg(
                    api.property.clone(),
                ))
            },

            cmd_in: if api.cmd_in.is_empty() {
                None
            } else {
                Some(get_designer_api_cmd_likes_from_pkg(api.cmd_in.clone()))
            },
            cmd_out: if api.cmd_out.is_empty() {
                None
            } else {
                Some(get_designer_api_cmd_likes_from_pkg(api.cmd_out.clone()))
            },

            data_in: if api.data_in.is_empty() {
                None
            } else {
                Some(get_designer_api_data_likes_from_pkg(api.data_in.clone()))
            },
            data_out: if api.data_out.is_empty() {
                None
            } else {
                Some(get_designer_api_data_likes_from_pkg(api.data_out.clone()))
            },

            audio_frame_in: if api.audio_frame_in.is_empty() {
                None
            } else {
                Some(get_designer_api_data_likes_from_pkg(
                    api.audio_frame_in.clone(),
                ))
            },
            audio_frame_out: if api.audio_frame_out.is_empty() {
                None
            } else {
                Some(get_designer_api_data_likes_from_pkg(
                    api.audio_frame_out.clone(),
                ))
            },

            video_frame_in: if api.video_frame_in.is_empty() {
                None
            } else {
                Some(get_designer_api_data_likes_from_pkg(
                    api.video_frame_in.clone(),
                ))
            },
            video_frame_out: if api.video_frame_out.is_empty() {
                None
            } else {
                Some(get_designer_api_data_likes_from_pkg(
                    api.video_frame_out.clone(),
                ))
            },
        }),
    }
}

pub async fn get_extension_addons(
    state: web::Data<Arc<RwLock<DesignerState>>>,
) -> impl Responder {
    let mut state = state.write().unwrap();

    match retrieve_extension_addons(&mut state) {
        Ok(extensions) => {
            let response = ApiResponse {
                status: Status::Ok,
                data: extensions,
                meta: None,
            };
            HttpResponse::Ok().json(response)
        }
        Err(error_response) => HttpResponse::BadRequest().json(error_response),
    }
}

pub async fn get_extension_addon_by_name(
    state: web::Data<Arc<RwLock<DesignerState>>>,
    path: web::Path<String>, // NEW
) -> impl Responder {
    let addon_name = path.into_inner(); // NEW
    let mut state = state.write().unwrap();

    match retrieve_extension_addons(&mut state) {
        Ok(extensions) => {
            if let Some(extension) = extensions
                .into_iter()
                .find(|ext| ext.addon_name == addon_name)
            {
                let response = ApiResponse {
                    status: Status::Ok,
                    data: extension,
                    meta: None,
                };
                HttpResponse::Ok().json(response)
            } else {
                let error_response = ErrorResponse {
                    status: Status::Fail,
                    message: format!(
                        "Extension addon with name '{}' not found",
                        addon_name
                    ),
                    error: None,
                };
                HttpResponse::NotFound().json(error_response)
            }
        }
        Err(error_response) => HttpResponse::BadRequest().json(error_response),
    }
}

#[cfg(test)]
mod tests {
    use crate::{
        config::TmanConfig,
        designer::{
            graphs::nodes::{
                DesignerApiCmdLike, DesignerApiDataLike,
                DesignerPropertyAttributes, DesignerPropertyItem,
            },
            mock::tests::inject_all_pkgs_for_mock,
        },
    };

    use super::*;
    use actix_web::{test, App};

    #[actix_web::test]
    async fn test_get_extension_addons() {
        let mut designer_state = DesignerState {
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
            inject_all_pkgs_for_mock(&mut designer_state, all_pkgs_json);
        assert!(inject_ret.is_ok());

        let designer_state = Arc::new(RwLock::new(designer_state));

        let app = test::init_service(
            App::new().app_data(web::Data::new(designer_state)).route(
                "/api/designer/v1/addons/extensions",
                web::get().to(get_extension_addons),
            ),
        )
        .await;

        let req = test::TestRequest::get()
            .uri("/api/designer/v1/addons/extensions")
            .to_request();
        let resp = test::call_service(&app, req).await;

        assert!(resp.status().is_success());

        let body = test::read_body(resp).await;
        let body_str = std::str::from_utf8(&body).unwrap();

        let addons: ApiResponse<Vec<DesignerExtensionAddon>> =
            serde_json::from_str(body_str).unwrap();

        let expected_addons = vec![
            DesignerExtensionAddon {
                addon_name: "extension_addon_1".to_string(),
                url: "".to_string(),
                api: Some(DesignerApi {
                    property: None,
                    cmd_in: None,
                    cmd_out: Some(vec![
                        DesignerApiCmdLike {
                            name: "test_cmd".to_string(),
                            property: Some(vec![DesignerPropertyItem {
                                name: "test_property".to_string(),
                                attributes: DesignerPropertyAttributes {
                                    prop_type: "int8".to_string(),
                                },
                            }]),
                            required: None,
                            result: None,
                        },
                        DesignerApiCmdLike {
                            name: "has_required".to_string(),
                            property: Some(vec![DesignerPropertyItem {
                                name: "foo".to_string(),
                                attributes: DesignerPropertyAttributes {
                                    prop_type: "string".to_string(),
                                },
                            }]),
                            required: Some(vec!["foo".to_string()]),
                            result: None,
                        },
                        DesignerApiCmdLike {
                            name: "has_required_mismatch".to_string(),
                            property: Some(vec![DesignerPropertyItem {
                                name: "foo".to_string(),
                                attributes: DesignerPropertyAttributes {
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
            DesignerExtensionAddon {
                addon_name: "extension_addon_2".to_string(),
                url: "".to_string(),
                api: Some(DesignerApi {
                    property: None,
                    cmd_in: Some(vec![
                        DesignerApiCmdLike {
                            name: "test_cmd".to_string(),
                            property: Some(vec![DesignerPropertyItem {
                                name: "test_property".to_string(),
                                attributes: DesignerPropertyAttributes {
                                    prop_type: "int32".to_string(),
                                },
                            }]),
                            required: None,
                            result: None,
                        },
                        DesignerApiCmdLike {
                            name: "another_test_cmd".to_string(),
                            property: Some(vec![DesignerPropertyItem {
                                name: "test_property".to_string(),
                                attributes: DesignerPropertyAttributes {
                                    prop_type: "int8".to_string(),
                                },
                            }]),
                            required: None,
                            result: None,
                        },
                        DesignerApiCmdLike {
                            name: "has_required".to_string(),
                            property: Some(vec![DesignerPropertyItem {
                                name: "foo".to_string(),
                                attributes: DesignerPropertyAttributes {
                                    prop_type: "string".to_string(),
                                },
                            }]),
                            required: Some(vec!["foo".to_string()]),
                            result: None,
                        },
                        DesignerApiCmdLike {
                            name: "has_required_mismatch".to_string(),
                            property: Some(vec![DesignerPropertyItem {
                                name: "foo".to_string(),
                                attributes: DesignerPropertyAttributes {
                                    prop_type: "string".to_string(),
                                },
                            }]),
                            required: None,
                            result: None,
                        },
                    ]),
                    cmd_out: None,
                    data_in: None,
                    data_out: Some(vec![DesignerApiDataLike {
                        name: "data_has_required".to_string(),
                        property: Some(vec![DesignerPropertyItem {
                            name: "foo".to_string(),
                            attributes: DesignerPropertyAttributes {
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
            DesignerExtensionAddon {
                addon_name: "extension_addon_3".to_string(),
                url: "".to_string(),
                api: Some(DesignerApi {
                    property: None,
                    cmd_in: Some(vec![DesignerApiCmdLike {
                        name: "test_cmd".to_string(),
                        property: Some(vec![DesignerPropertyItem {
                            name: "test_property".to_string(),
                            attributes: DesignerPropertyAttributes {
                                prop_type: "string".to_string(),
                            },
                        }]),
                        required: None,
                        result: None,
                    }]),
                    cmd_out: None,
                    data_in: Some(vec![DesignerApiDataLike {
                        name: "data_has_required".to_string(),
                        property: Some(vec![DesignerPropertyItem {
                            name: "foo".to_string(),
                            attributes: DesignerPropertyAttributes {
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

        let json: ApiResponse<Vec<DesignerExtensionAddon>> =
            serde_json::from_str(body_str).unwrap();
        let pretty_json = serde_json::to_string_pretty(&json).unwrap();
        println!("Response body: {}", pretty_json);
    }

    #[actix_web::test]
    async fn test_get_extension_addon_by_name() {
        let mut designer_state = DesignerState {
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
            inject_all_pkgs_for_mock(&mut designer_state, all_pkgs_json);
        assert!(inject_ret.is_ok());

        let designer_state = Arc::new(RwLock::new(designer_state));

        let app = test::init_service(
            App::new()
                .app_data(web::Data::new(designer_state.clone()))
                .route(
                    "/api/designer/v1/addons/extensions/{name}",
                    web::get().to(get_extension_addon_by_name),
                ),
        )
        .await;

        let req = test::TestRequest::get()
            .uri("/api/designer/v1/addons/extensions/extension_addon_1")
            .to_request();
        let resp = test::call_service(&app, req).await;

        assert!(resp.status().is_success());

        let body = test::read_body(resp).await;
        let body_str = std::str::from_utf8(&body).unwrap();

        let addon: ApiResponse<DesignerExtensionAddon> =
            serde_json::from_str(body_str).unwrap();

        let expected_addon = DesignerExtensionAddon {
            addon_name: "extension_addon_1".to_string(),
            url: "".to_string(),
            api: Some(DesignerApi {
                property: None,
                cmd_in: None,
                cmd_out: Some(vec![
                    DesignerApiCmdLike {
                        name: "test_cmd".to_string(),
                        property: Some(vec![DesignerPropertyItem {
                            name: "test_property".to_string(),
                            attributes: DesignerPropertyAttributes {
                                prop_type: "int8".to_string(),
                            },
                        }]),
                        required: None,
                        result: None,
                    },
                    DesignerApiCmdLike {
                        name: "has_required".to_string(),
                        property: Some(vec![DesignerPropertyItem {
                            name: "foo".to_string(),
                            attributes: DesignerPropertyAttributes {
                                prop_type: "string".to_string(),
                            },
                        }]),
                        required: Some(vec!["foo".to_string()]),
                        result: None,
                    },
                    DesignerApiCmdLike {
                        name: "has_required_mismatch".to_string(),
                        property: Some(vec![DesignerPropertyItem {
                            name: "foo".to_string(),
                            attributes: DesignerPropertyAttributes {
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
        };

        assert_eq!(addon.data, expected_addon);

        let req = test::TestRequest::get()
            .uri("/api/designer/v1/addons/extensions/non_existent_addon")
            .to_request();
        let resp = test::call_service(&app, req).await;

        assert_eq!(resp.status(), actix_web::http::StatusCode::NOT_FOUND);

        let body = test::read_body(resp).await;
        let body_str = std::str::from_utf8(&body).unwrap();

        let error_response: ErrorResponse =
            serde_json::from_str(body_str).unwrap();
        assert_eq!(
            error_response.message,
            "Extension addon with name 'non_existent_addon' not found"
        );
    }
}
