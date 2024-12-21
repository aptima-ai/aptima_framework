//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
use serde::{Deserialize, Serialize};

use crate::pkg_info::dependencies::PkgDependency;

#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct ManifestDependency {
    #[serde(rename = "type")]
    pub pkg_type: String,
    pub name: String,
    pub version: String,
}

impl From<&PkgDependency> for ManifestDependency {
    fn from(pkg_dependency: &PkgDependency) -> Self {
        ManifestDependency {
            pkg_type: pkg_dependency.type_and_name.pkg_type.to_string(),
            name: pkg_dependency.type_and_name.name.clone(),
            version: pkg_dependency.version_req.to_string(),
        }
    }
}
