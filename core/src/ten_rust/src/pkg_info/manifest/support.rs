//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
use serde::{Deserialize, Serialize};

use crate::pkg_info::supports::PkgSupport;

#[derive(Serialize, Deserialize, Clone, Debug)]
pub struct ManifestSupport {
    #[serde(skip_serializing_if = "Option::is_none")]
    pub os: Option<String>,

    #[serde(skip_serializing_if = "Option::is_none")]
    pub arch: Option<String>,
}

impl From<&PkgSupport> for ManifestSupport {
    fn from(support: &PkgSupport) -> Self {
        ManifestSupport {
            os: support.os.as_ref().map(|_os| _os.to_string()),
            arch: support.arch.as_ref().map(|_arch| _arch.to_string()),
        }
    }
}
