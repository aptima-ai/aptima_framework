//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
use std::{
    collections::HashMap,
    sync::{Arc, RwLock},
};

use actix_web::{web, HttpResponse, Responder};
use serde::{Deserialize, Serialize};

use crate::dev_server::common::{
    get_dev_server_api_cmd_likes_from_pkg,
    get_dev_server_api_data_likes_from_pkg,
    get_dev_server_property_hashmap_from_pkg,
};
use crate::dev_server::get_all_pkgs::get_all_pkgs;
use crate::dev_server::response::{ApiResponse, ErrorResponse, Status};
use crate::dev_server::DevServerState;
use ten_rust::pkg_info::api::{
    PkgApiCmdLike, PkgApiDataLike, PkgPropertyAttributes, PkgPropertyItem,
};
use ten_rust::pkg_info::predefined_graphs::extension::get_extension_nodes_in_graph;
use ten_rust::pkg_info::{
    api::PkgCmdResult, predefined_graphs::extension::get_pkg_info_for_extension,
};

#[derive(Serialize, Deserialize, Debug, PartialEq, Clone)]
pub struct DevServerExtension {
    pub addon: String,
    pub name: String,

    // The extension group which this extension belongs.
    pub extension_group: String,

    // The app which this extension belongs.
    pub app: String,

    #[serde(skip_serializing_if = "Option::is_none")]
    pub api: Option<DevServerApi>,

    pub property: Option<serde_json::Value>,
}

#[derive(Serialize, Deserialize, Debug, PartialEq, Clone)]
pub struct DevServerApi {
    #[serde(skip_serializing_if = "Option::is_none")]
    pub property: Option<HashMap<String, DevServerPropertyAttributes>>,

    #[serde(skip_serializing_if = "Option::is_none")]
    pub cmd_in: Option<Vec<DevServerApiCmdLike>>,

    #[serde(skip_serializing_if = "Option::is_none")]
    pub cmd_out: Option<Vec<DevServerApiCmdLike>>,

    #[serde(skip_serializing_if = "Option::is_none")]
    pub data_in: Option<Vec<DevServerApiDataLike>>,

    #[serde(skip_serializing_if = "Option::is_none")]
    pub data_out: Option<Vec<DevServerApiDataLike>>,

    #[serde(skip_serializing_if = "Option::is_none")]
    pub audio_frame_in: Option<Vec<DevServerApiDataLike>>,

    #[serde(skip_serializing_if = "Option::is_none")]
    pub audio_frame_out: Option<Vec<DevServerApiDataLike>>,

    #[serde(skip_serializing_if = "Option::is_none")]
    pub video_frame_in: Option<Vec<DevServerApiDataLike>>,

    #[serde(skip_serializing_if = "Option::is_none")]
    pub video_frame_out: Option<Vec<DevServerApiDataLike>>,
}

#[derive(Serialize, Deserialize, Debug, Clone, PartialEq)]
pub struct DevServerPropertyAttributes {
    #[serde(rename = "type")]
    pub prop_type: String,
}

#[derive(Serialize, Deserialize, Debug, Clone, PartialEq)]
pub struct DevServerPropertyItem {
    pub name: String,
    pub attributes: DevServerPropertyAttributes,
}

impl From<PkgPropertyAttributes> for DevServerPropertyAttributes {
    fn from(api_property: PkgPropertyAttributes) -> Self {
        DevServerPropertyAttributes {
            prop_type: api_property.prop_type.to_string(),
        }
    }
}

impl From<PkgPropertyItem> for DevServerPropertyItem {
    fn from(item: PkgPropertyItem) -> Self {
        DevServerPropertyItem {
            name: item.name,
            attributes: item.attributes.into(),
        }
    }
}

#[derive(Serialize, Deserialize, Debug, Clone, PartialEq)]
pub struct DevServerCmdResult {
    pub property: Vec<DevServerPropertyItem>,

    #[serde(skip_serializing_if = "Option::is_none")]
    pub required: Option<Vec<String>>,
}

impl From<PkgCmdResult> for DevServerCmdResult {
    fn from(cmd_result: PkgCmdResult) -> Self {
        DevServerCmdResult {
            property: cmd_result
                .property
                .into_iter()
                .map(|v| v.into())
                .collect(),
            required: if cmd_result.required.is_empty() {
                None
            } else {
                Some(cmd_result.required)
            },
        }
    }
}

#[derive(Serialize, Deserialize, Debug, PartialEq, Clone)]
pub struct DevServerApiCmdLike {
    pub name: String,

    #[serde(skip_serializing_if = "Option::is_none")]
    pub property: Option<Vec<DevServerPropertyItem>>,

    #[serde(skip_serializing_if = "Option::is_none")]
    pub required: Option<Vec<String>>,

    #[serde(skip_serializing_if = "Option::is_none")]
    pub result: Option<DevServerCmdResult>,
}

impl From<PkgApiCmdLike> for DevServerApiCmdLike {
    fn from(api_cmd_like: PkgApiCmdLike) -> Self {
        DevServerApiCmdLike {
            name: api_cmd_like.name,
            property: if api_cmd_like.property.is_empty() {
                None
            } else {
                Some(get_dev_server_property_items_from_pkg(
                    api_cmd_like.property,
                ))
            },
            required: if api_cmd_like.required.is_empty() {
                None
            } else {
                Some(api_cmd_like.required)
            },
            result: if api_cmd_like.result.property.is_empty() {
                None
            } else {
                Some(api_cmd_like.result.into())
            },
        }
    }
}

#[derive(Serialize, Deserialize, Debug, PartialEq, Clone)]
pub struct DevServerApiDataLike {
    pub name: String,

    #[serde(skip_serializing_if = "Option::is_none")]
    pub property: Option<Vec<DevServerPropertyItem>>,

    #[serde(skip_serializing_if = "Option::is_none")]
    pub required: Option<Vec<String>>,
}

impl From<PkgApiDataLike> for DevServerApiDataLike {
    fn from(api_data_like: PkgApiDataLike) -> Self {
        DevServerApiDataLike {
            name: api_data_like.name,
            property: if api_data_like.property.is_empty() {
                None
            } else {
                Some(get_dev_server_property_items_from_pkg(
                    api_data_like.property,
                ))
            },
            required: if api_data_like.required.is_empty() {
                None
            } else {
                Some(api_data_like.required)
            },
        }
    }
}

fn get_dev_server_property_items_from_pkg(
    items: Vec<PkgPropertyItem>,
) -> Vec<DevServerPropertyItem> {
    items.into_iter().map(|v| v.into()).collect()
}

pub async fn get_graph_nodes(
    state: web::Data<Arc<RwLock<DevServerState>>>,
    path: web::Path<String>,
) -> impl Responder {
    let graph_name = path.into_inner();
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
            match get_extension_nodes_in_graph(&graph_name, all_pkgs) {
                Ok(exts) => exts,
                Err(err) => {
                    let error_response = ErrorResponse {
                        status: Status::Fail,
                        message: format!(
                        "Error fetching runtime extensions for graph '{}': {}",
                        graph_name, err
                    ),
                        error: None,
                    };
                    return HttpResponse::NotFound().json(error_response);
                }
            };

        let mut resp_extensions: Vec<DevServerExtension> = Vec::new();
        for extension in &extensions {
            let pkg_info = get_pkg_info_for_extension(extension, all_pkgs);
            if let Err(err) = pkg_info {
                let error_response = ErrorResponse {
                    status: Status::Fail,
                    message: format!(
                        "Error fetching runtime extensions for graph '{}': {}",
                        graph_name, err
                    ),
                    error: None,
                };
                return HttpResponse::NotFound().json(error_response);
            }

            let pkg_info = pkg_info.unwrap();
            resp_extensions.push(DevServerExtension {
                addon: extension.addon.clone(),
                name: extension.name.clone(),
                extension_group: extension.extension_group.clone().unwrap(),
                app: extension.app.clone(),
                api: pkg_info.api.as_ref().map(|api| DevServerApi {
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
                }),
                property: extension.property.clone(),
            });
        }

        let response = ApiResponse {
            status: Status::Ok,
            data: resp_extensions,
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
    async fn test_get_extensions_success() {
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
                "/api/dev-server/v1/graphs/{graph_name}/nodes",
                web::get().to(get_graph_nodes),
            ),
        )
        .await;

        let req = test::TestRequest::get()
            .uri("/api/dev-server/v1/graphs/0/nodes")
            .to_request();
        let resp = test::call_service(&app, req).await;

        assert!(resp.status().is_success());

        let body = test::read_body(resp).await;
        let body_str = std::str::from_utf8(&body).unwrap();

        let extensions: ApiResponse<Vec<DevServerExtension>> =
            serde_json::from_str(body_str).unwrap();

        // Create the expected Version struct
        let expected_extensions = vec![
            DevServerExtension {
                addon: "extension_addon_1".to_string(),
                name: "extension_1".to_string(),
                extension_group: "extension_group_1".to_string(),
                app: default_app_loc(),
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
                property: None,
            },
            DevServerExtension {
                addon: "extension_addon_2".to_string(),
                name: "extension_2".to_string(),
                extension_group: "extension_group_1".to_string(),
                app: default_app_loc(),
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
                property: Some(json!({
                    "a": 1
                })),
            },
            DevServerExtension {
                addon: "extension_addon_3".to_string(),
                name: "extension_3".to_string(),
                extension_group: "extension_group_1".to_string(),
                app: default_app_loc(),
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
                property: None,
            },
        ];

        assert_eq!(extensions.data, expected_extensions);
        assert!(!extensions.data.is_empty());

        let json: serde_json::Value = serde_json::from_str(body_str).unwrap();
        let pretty_json = serde_json::to_string_pretty(&json).unwrap();
        println!("Response body: {}", pretty_json);
    }

    #[actix_web::test]
    async fn test_get_extensions_no_graph() {
        let dev_server_state = Arc::new(RwLock::new(DevServerState {
            base_dir: None,
            all_pkgs: Some(vec![]),
            tman_config: TmanConfig::default(),
        }));

        let app = test::init_service(
            App::new().app_data(web::Data::new(dev_server_state)).route(
                "/api/dev-server/v1/graphs/{graph_name}/extensions",
                web::get().to(get_graph_nodes),
            ),
        )
        .await;

        let req = test::TestRequest::get()
            .uri("/api/dev-server/v1/graphs/no_existing_graph/extensions")
            .to_request();
        let resp = test::call_service(&app, req).await;

        assert!(resp.status().is_client_error());

        let body = test::read_body(resp).await;
        let body_str = std::str::from_utf8(&body).unwrap();

        let error: ErrorResponse = serde_json::from_str(body_str).unwrap();

        assert!(error
            .message
            .contains("Error fetching runtime extensions for graph"));
    }

    #[actix_web::test]
    async fn test_get_extensions_has_wrong_addon() {
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
                "/api/dev-server/v1/graphs/{graph_name}/extensions",
                web::get().to(get_graph_nodes),
            ),
        )
        .await;

        let req = test::TestRequest::get()
            .uri("/api/dev-server/v1/graphs/addon_not_found/extensions")
            .to_request();

        let resp = test::call_service(&app, req).await;

        assert!(resp.status().is_client_error());

        let body = test::read_body(resp).await;
        let body_str = std::str::from_utf8(&body).unwrap();

        let error: ErrorResponse = serde_json::from_str(body_str).unwrap();

        assert!(error
            .message
            .contains("the addon 'extension_addon_1_not_found' used to instantiate extension 'extension_1' is not found"));
    }
}
