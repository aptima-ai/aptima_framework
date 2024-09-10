//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
use std::hash::{Hash, Hasher};

use anyhow::Result;
use serde::{Deserialize, Serialize};

use super::pkg_type::PkgType;
use crate::pkg_info::manifest::Manifest;

#[derive(Clone, Debug, Serialize, Deserialize)]
pub struct PkgIdentity {
    pub pkg_type: PkgType,
    pub name: String,
}

impl Hash for PkgIdentity {
    fn hash<H: Hasher>(&self, state: &mut H) {
        self.pkg_type.hash(state);
        self.name.hash(state);
    }
}

impl PartialEq for PkgIdentity {
    fn eq(&self, other: &Self) -> bool {
        self.pkg_type == other.pkg_type && self.name == other.name
    }
}

impl Eq for PkgIdentity {}

impl PkgIdentity {
    pub fn from_manifest(manifest: &Manifest) -> Result<Self> {
        Ok(PkgIdentity {
            pkg_type: manifest.pkg_type.parse::<PkgType>()?,
            name: manifest.name.clone(),
        })
    }
}
