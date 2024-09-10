//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
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

impl From<PkgDependency> for ManifestDependency {
    fn from(pkg_dependency: PkgDependency) -> Self {
        ManifestDependency {
            pkg_type: pkg_dependency.pkg_identity.pkg_type.to_string(),
            name: pkg_dependency.pkg_identity.name,
            version: pkg_dependency.version_req.to_string(),
        }
    }
}
