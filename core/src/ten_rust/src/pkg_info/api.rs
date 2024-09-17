//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
use std::{collections::HashMap, str::FromStr};

use anyhow::Result;
use serde::{Deserialize, Serialize};

use super::value_type::ValueType;
use crate::pkg_info::manifest::{
    api::{
        ManifestApi, ManifestApiCmdLike, ManifestApiDataLike,
        ManifestCmdResult, ManifestPropertyAttributes, ManifestPropertyItem,
    },
    Manifest,
};

#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct PkgApi {
    pub property: HashMap<String, PkgPropertyAttributes>,
    pub required: Vec<String>,

    pub cmd_in: Vec<PkgApiCmdLike>,
    pub cmd_out: Vec<PkgApiCmdLike>,

    pub data_in: Vec<PkgApiDataLike>,
    pub data_out: Vec<PkgApiDataLike>,

    pub audio_frame_in: Vec<PkgApiDataLike>,
    pub audio_frame_out: Vec<PkgApiDataLike>,

    pub video_frame_in: Vec<PkgApiDataLike>,
    pub video_frame_out: Vec<PkgApiDataLike>,
}

impl From<ManifestApi> for PkgApi {
    fn from(manifest_api: ManifestApi) -> Self {
        PkgApi {
            property: manifest_api
                .property
                .clone()
                .unwrap_or_default()
                .into_iter()
                .map(|(k, v)| (k, v.into()))
                .collect(),
            required: manifest_api.required.unwrap_or_default(),
            cmd_in: manifest_api
                .cmd_in
                .clone()
                .unwrap_or_default()
                .into_iter()
                .map(|v| v.into())
                .collect(),
            cmd_out: manifest_api
                .cmd_out
                .clone()
                .unwrap_or_default()
                .into_iter()
                .map(|v| v.into())
                .collect(),

            data_in: manifest_api
                .data_in
                .clone()
                .unwrap_or_default()
                .into_iter()
                .map(|v| v.into())
                .collect(),
            data_out: manifest_api
                .data_out
                .clone()
                .unwrap_or_default()
                .into_iter()
                .map(|v| v.into())
                .collect(),

            audio_frame_in: manifest_api
                .audio_frame_in
                .clone()
                .unwrap_or_default()
                .into_iter()
                .map(|v| v.into())
                .collect(),
            audio_frame_out: manifest_api
                .audio_frame_out
                .clone()
                .unwrap_or_default()
                .into_iter()
                .map(|v| v.into())
                .collect(),

            video_frame_in: manifest_api
                .video_frame_in
                .clone()
                .unwrap_or_default()
                .into_iter()
                .map(|v| v.into())
                .collect(),
            video_frame_out: manifest_api
                .video_frame_out
                .clone()
                .unwrap_or_default()
                .into_iter()
                .map(|v| v.into())
                .collect(),
        }
    }
}

impl PkgApi {
    pub fn from_manifest(manifest: &Manifest) -> Result<Option<Self>> {
        if let Some(api) = &manifest.api {
            let pkg_api: PkgApi = api.clone().into();
            Ok(Some(pkg_api))
        } else {
            Ok(None)
        }
    }
}

#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct PkgPropertyAttributes {
    #[serde(rename = "type")]
    pub prop_type: ValueType,
}

impl From<ManifestPropertyAttributes> for PkgPropertyAttributes {
    fn from(manifest_property_attributes: ManifestPropertyAttributes) -> Self {
        PkgPropertyAttributes {
            prop_type: ValueType::from_str(
                &manifest_property_attributes.prop_type,
            )
            .unwrap(),
        }
    }
}

#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct PkgPropertyItem {
    pub name: String,

    // TODO(Liu): Add more attributes for composite types.
    pub attributes: PkgPropertyAttributes,
}

impl From<ManifestPropertyItem> for PkgPropertyItem {
    fn from(manifest_property_item: ManifestPropertyItem) -> Self {
        PkgPropertyItem {
            name: manifest_property_item.name,
            attributes: manifest_property_item.attributes.into(),
        }
    }
}

#[derive(Serialize, Deserialize, Debug, Clone, Default)]
pub struct PkgCmdResult {
    pub property: Vec<PkgPropertyItem>,
    pub required: Vec<String>,
}

fn get_pkg_cmd_result_from_manifest(
    manifest_cmd_result: Option<ManifestCmdResult>,
) -> PkgCmdResult {
    match manifest_cmd_result {
        Some(manifest_cmd_result) => PkgCmdResult {
            property: manifest_cmd_result
                .property
                .into_iter()
                .map(|v| v.into())
                .collect(),
            required: manifest_cmd_result.required.unwrap_or_default(),
        },
        None => PkgCmdResult::default(),
    }
}

#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct PkgApiCmdLike {
    pub name: String,
    pub property: Vec<PkgPropertyItem>,
    pub required: Vec<String>,
    pub result: PkgCmdResult,
}

impl From<ManifestApiCmdLike> for PkgApiCmdLike {
    fn from(manifest_cmd: ManifestApiCmdLike) -> Self {
        PkgApiCmdLike {
            name: manifest_cmd.name,
            property: manifest_cmd
                .property
                .unwrap_or_default()
                .into_iter()
                .map(|v| v.into())
                .collect(),
            required: manifest_cmd.required.unwrap_or_default(),
            result: get_pkg_cmd_result_from_manifest(manifest_cmd.result),
        }
    }
}

#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct PkgApiDataLike {
    pub name: String,
    pub property: Vec<PkgPropertyItem>,
    pub required: Vec<String>,
}

impl From<ManifestApiDataLike> for PkgApiDataLike {
    fn from(manifest_data: ManifestApiDataLike) -> Self {
        PkgApiDataLike {
            name: manifest_data.name,
            property: manifest_data
                .property
                .unwrap_or_default()
                .into_iter()
                .map(|v| v.into())
                .collect(),
            required: manifest_data.required.unwrap_or_default(),
        }
    }
}
