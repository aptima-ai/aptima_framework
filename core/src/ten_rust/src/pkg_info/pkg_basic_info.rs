//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
use std::hash::{Hash, Hasher};

use semver::Version;

use super::{pkg_type_and_name::PkgTypeAndName, supports::PkgSupport, PkgInfo};

#[derive(Clone, Debug, Eq)]
pub struct PkgBasicInfo {
    pub type_and_name: PkgTypeAndName,
    pub version: Version,

    // Since the declaration 'does not support all environments' has no
    // practical meaning, not specifying the 'supports' field or specifying an
    // empty 'supports' field both represent support for all environments.
    // Therefore, the 'supports' field here is not an option. An empty
    // 'supports' field represents support for all combinations of
    // environments.
    pub supports: Vec<PkgSupport>,
}

impl Hash for PkgBasicInfo {
    fn hash<H: Hasher>(&self, state: &mut H) {
        self.type_and_name.pkg_type.hash(state);
        self.type_and_name.name.hash(state);
        self.version.hash(state);
        self.supports.hash(state);
    }
}

impl PartialEq for PkgBasicInfo {
    fn eq(&self, other: &Self) -> bool {
        self.type_and_name.pkg_type == other.type_and_name.pkg_type
            && self.type_and_name.name == other.type_and_name.name
            && self.version == other.version
            && self.supports == other.supports
    }
}

impl From<&PkgInfo> for PkgBasicInfo {
    fn from(pkg_info: &PkgInfo) -> Self {
        PkgBasicInfo {
            type_and_name: pkg_info.into(),
            version: pkg_info.version.clone(),
            supports: pkg_info.supports.clone(),
        }
    }
}
