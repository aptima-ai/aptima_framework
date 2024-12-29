//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
use std::{collections::HashMap, fmt};

use linked_hash_map::LinkedHashMap;
use regex::Regex;
use serde::{
    de::{self, Visitor},
    Deserialize, Deserializer, Serialize,
};

use crate::pkg_info::api::{
    PkgApi, PkgApiCmdLike, PkgApiDataLike, PkgCmdResult, PkgPropertyAttributes,
    PkgPropertyItem,
};

#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct ManifestApi {
    #[serde(skip_serializing_if = "Option::is_none")]
    pub property: Option<HashMap<String, ManifestPropertyAttributes>>,

    // Only manifest.json of extension has this field.
    #[serde(skip_serializing_if = "Option::is_none")]
    pub required: Option<Vec<String>>,

    #[serde(skip_serializing_if = "Option::is_none")]
    pub cmd_in: Option<Vec<ManifestApiCmdLike>>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub cmd_out: Option<Vec<ManifestApiCmdLike>>,

    #[serde(skip_serializing_if = "Option::is_none")]
    pub data_in: Option<Vec<ManifestApiDataLike>>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub data_out: Option<Vec<ManifestApiDataLike>>,

    #[serde(skip_serializing_if = "Option::is_none")]
    pub audio_frame_in: Option<Vec<ManifestApiDataLike>>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub audio_frame_out: Option<Vec<ManifestApiDataLike>>,

    #[serde(skip_serializing_if = "Option::is_none")]
    pub video_frame_in: Option<Vec<ManifestApiDataLike>>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub video_frame_out: Option<Vec<ManifestApiDataLike>>,
}

#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct ManifestPropertyItem {
    pub name: String,
    pub attributes: ManifestPropertyAttributes,
}

#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct ManifestPropertyAttributes {
    #[serde(rename = "type")]
    pub prop_type: String,
}

#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct ManifestCmdResult {
    #[serde(deserialize_with = "deserialize_property_items", default)]
    pub property: Vec<ManifestPropertyItem>,

    #[serde(deserialize_with = "deserialize_optional_required", default)]
    pub required: Option<Vec<String>>,
}

#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct ManifestApiCmdLike {
    #[serde(deserialize_with = "validate_msg_name")]
    pub name: String,

    #[serde(deserialize_with = "deserialize_optional_property_items", default)]
    pub property: Option<Vec<ManifestPropertyItem>>,

    #[serde(deserialize_with = "deserialize_optional_required", default)]
    pub required: Option<Vec<String>>,

    pub result: Option<ManifestCmdResult>,
}

#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct ManifestApiDataLike {
    #[serde(deserialize_with = "validate_msg_name")]
    pub name: String,

    #[serde(deserialize_with = "deserialize_optional_property_items", default)]
    pub property: Option<Vec<ManifestPropertyItem>>,

    #[serde(deserialize_with = "deserialize_optional_required", default)]
    pub required: Option<Vec<String>>,
}

fn validate_msg_name<'de, D>(deserializer: D) -> Result<String, D::Error>
where
    D: Deserializer<'de>,
{
    let msg_name: String = String::deserialize(deserializer)?;
    let re = Regex::new(r"^[A-Za-z_][A-Za-z0-9_]*$").unwrap();
    if re.is_match(&msg_name) {
        Ok(msg_name)
    } else {
        Err(serde::de::Error::custom("Invalid message name format, it needs to conform to the pattern ^[A-Za-z_][A-Za-z0-9_]*$"))
    }
}

fn deserialize_property_items<'de, D>(
    deserializer: D,
) -> Result<Vec<ManifestPropertyItem>, D::Error>
where
    D: Deserializer<'de>,
{
    let map: LinkedHashMap<String, ManifestPropertyAttributes> =
        Deserialize::deserialize(deserializer)?;

    let mut items: Vec<ManifestPropertyItem> = Vec::new();
    for (key, value) in map.into_iter() {
        items.push(ManifestPropertyItem {
            name: key,
            attributes: value,
        });
    }

    Ok(items)
}

fn deserialize_optional_property_items<'de, D>(
    deserializer: D,
) -> Result<Option<Vec<ManifestPropertyItem>>, D::Error>
where
    D: Deserializer<'de>,
{
    deserializer.deserialize_option(PropertyVisitor)
}

struct PropertyVisitor;

impl<'de> Visitor<'de> for PropertyVisitor {
    type Value = Option<Vec<ManifestPropertyItem>>;

    fn expecting(&self, formatter: &mut fmt::Formatter) -> fmt::Result {
        formatter.write_str("an optional map of property items")
    }

    fn visit_none<E>(self) -> Result<Self::Value, E>
    where
        E: de::Error,
    {
        Ok(None)
    }

    fn visit_some<D>(self, deserializer: D) -> Result<Self::Value, D::Error>
    where
        D: Deserializer<'de>,
    {
        // Preserve the insertion order. Because automatic code generation might
        // be needed in the future, the order defined within the API is very
        // important. Use deserialization methods that can preserve this order.
        let map: LinkedHashMap<String, ManifestPropertyAttributes> =
            Deserialize::deserialize(deserializer)?;

        let mut items: Vec<ManifestPropertyItem> = Vec::new();
        for (key, value) in map.into_iter() {
            items.push(ManifestPropertyItem {
                name: key,
                attributes: value,
            });
        }

        Ok(Some(items))
    }
}

fn deserialize_optional_required<'de, D>(
    deserializer: D,
) -> Result<Option<Vec<String>>, D::Error>
where
    D: Deserializer<'de>,
{
    deserializer.deserialize_option(PropertyRequiredVisitor)
}

struct PropertyRequiredVisitor;

impl<'de> Visitor<'de> for PropertyRequiredVisitor {
    type Value = Option<Vec<String>>;

    fn expecting(&self, formatter: &mut fmt::Formatter) -> fmt::Result {
        formatter.write_str("an optional list of acquired properties.")
    }

    fn visit_none<E>(self) -> Result<Self::Value, E>
    where
        E: de::Error,
    {
        Ok(None)
    }

    fn visit_some<D>(self, deserializer: D) -> Result<Self::Value, D::Error>
    where
        D: Deserializer<'de>,
    {
        let items: Vec<String> = Deserialize::deserialize(deserializer)?;

        if items.is_empty() {
            Err(serde::de::Error::custom(
                "the 'required' field should not be an empty list",
            ))
        } else {
            Ok(Some(items))
        }
    }
}

impl From<PkgApi> for ManifestApi {
    fn from(pkg_api: PkgApi) -> Self {
        ManifestApi {
            property: if pkg_api.property.is_empty() {
                None
            } else {
                Some(get_manifest_property_attributes_from_pkg(
                    pkg_api.property,
                ))
            },
            required: Some(pkg_api.required),
            cmd_in: if pkg_api.cmd_in.is_empty() {
                None
            } else {
                Some(get_manifest_api_cmd_like_from_pkg(pkg_api.cmd_in))
            },
            cmd_out: if pkg_api.cmd_out.is_empty() {
                None
            } else {
                Some(get_manifest_api_cmd_like_from_pkg(pkg_api.cmd_out))
            },
            data_in: if pkg_api.data_in.is_empty() {
                None
            } else {
                Some(get_manifest_api_data_like_from_pkg(pkg_api.data_in))
            },
            data_out: if pkg_api.data_out.is_empty() {
                None
            } else {
                Some(get_manifest_api_data_like_from_pkg(pkg_api.data_out))
            },
            audio_frame_in: if pkg_api.audio_frame_in.is_empty() {
                None
            } else {
                Some(get_manifest_api_data_like_from_pkg(
                    pkg_api.audio_frame_in,
                ))
            },
            audio_frame_out: if pkg_api.audio_frame_out.is_empty() {
                None
            } else {
                Some(get_manifest_api_data_like_from_pkg(
                    pkg_api.audio_frame_out,
                ))
            },
            video_frame_in: if pkg_api.video_frame_in.is_empty() {
                None
            } else {
                Some(get_manifest_api_data_like_from_pkg(
                    pkg_api.video_frame_in,
                ))
            },
            video_frame_out: if pkg_api.video_frame_out.is_empty() {
                None
            } else {
                Some(get_manifest_api_data_like_from_pkg(
                    pkg_api.video_frame_out,
                ))
            },
        }
    }
}

fn get_manifest_property_attributes_from_pkg(
    items: HashMap<String, PkgPropertyAttributes>,
) -> HashMap<String, ManifestPropertyAttributes> {
    items.into_iter().map(|(k, v)| (k, v.into())).collect()
}

fn get_manifest_api_cmd_like_from_pkg(
    pkg_api_cmd_like: Vec<PkgApiCmdLike>,
) -> Vec<ManifestApiCmdLike> {
    pkg_api_cmd_like.into_iter().map(|v| v.into()).collect()
}

fn get_manifest_api_data_like_from_pkg(
    pkg_api_cmd_like: Vec<PkgApiDataLike>,
) -> Vec<ManifestApiDataLike> {
    pkg_api_cmd_like.into_iter().map(|v| v.into()).collect()
}

impl From<PkgPropertyItem> for ManifestPropertyItem {
    fn from(pkg_property_item: PkgPropertyItem) -> Self {
        ManifestPropertyItem {
            name: pkg_property_item.name,
            attributes: pkg_property_item.attributes.into(),
        }
    }
}

impl From<PkgPropertyAttributes> for ManifestPropertyAttributes {
    fn from(pkg_property_item: PkgPropertyAttributes) -> Self {
        ManifestPropertyAttributes {
            prop_type: pkg_property_item.prop_type.to_string(),
        }
    }
}

fn get_manifest_property_items_from_pkg(
    pkg_property_items: Vec<PkgPropertyItem>,
) -> Vec<ManifestPropertyItem> {
    pkg_property_items.into_iter().map(|v| v.into()).collect()
}

impl From<PkgCmdResult> for ManifestCmdResult {
    fn from(pkg_cmd_result: PkgCmdResult) -> Self {
        ManifestCmdResult {
            property: get_manifest_property_items_from_pkg(
                pkg_cmd_result.property,
            ),
            required: Some(pkg_cmd_result.required),
        }
    }
}

impl From<PkgApiCmdLike> for ManifestApiCmdLike {
    fn from(pkg_api_cmd_like: PkgApiCmdLike) -> Self {
        ManifestApiCmdLike {
            name: pkg_api_cmd_like.name,
            property: if pkg_api_cmd_like.property.is_empty() {
                None
            } else {
                Some(get_manifest_property_items_from_pkg(
                    pkg_api_cmd_like.property,
                ))
            },
            required: if (pkg_api_cmd_like.required).is_empty() {
                None
            } else {
                Some(pkg_api_cmd_like.required)
            },
            result: if pkg_api_cmd_like.result.property.is_empty() {
                None
            } else {
                Some(pkg_api_cmd_like.result.into())
            },
        }
    }
}

impl From<PkgApiDataLike> for ManifestApiDataLike {
    fn from(pkg_api_data_like: PkgApiDataLike) -> Self {
        ManifestApiDataLike {
            name: pkg_api_data_like.name,
            property: if pkg_api_data_like.property.is_empty() {
                None
            } else {
                Some(get_manifest_property_items_from_pkg(
                    pkg_api_data_like.property,
                ))
            },
            required: if pkg_api_data_like.required.is_empty() {
                None
            } else {
                Some(pkg_api_data_like.required)
            },
        }
    }
}
