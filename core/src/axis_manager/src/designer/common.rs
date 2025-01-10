//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
use std::collections::HashMap;

use serde::Deserialize;

use axis_rust::pkg_info::api::{
    PkgApiCmdLike, PkgApiDataLike, PkgPropertyAttributes,
};

use super::graphs::nodes::{
    DesignerApiCmdLike, DesignerApiDataLike, DesignerPropertyAttributes,
};

pub fn get_designer_property_hashmap_from_pkg(
    items: HashMap<String, PkgPropertyAttributes>,
) -> HashMap<String, DesignerPropertyAttributes> {
    items.into_iter().map(|(k, v)| (k, v.into())).collect()
}

pub fn get_designer_api_cmd_likes_from_pkg(
    items: Vec<PkgApiCmdLike>,
) -> Vec<DesignerApiCmdLike> {
    items.into_iter().map(|v| v.into()).collect()
}

pub fn get_designer_api_data_likes_from_pkg(
    items: Vec<PkgApiDataLike>,
) -> Vec<DesignerApiDataLike> {
    items.into_iter().map(|v| v.into()).collect()
}

#[derive(Deserialize)]
pub struct CheckTypeQuery {
    #[serde(rename = "type")]
    pub check_type: String,
}
