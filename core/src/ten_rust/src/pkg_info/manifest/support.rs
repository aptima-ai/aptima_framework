//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
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
