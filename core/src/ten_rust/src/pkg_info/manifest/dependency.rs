//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
use serde::{Deserialize, Serialize};

use crate::pkg_info::{dependencies::PkgDependency, PkgInfo};

#[derive(Serialize, Deserialize, Debug, Clone)]
#[serde(untagged)]
pub enum ManifestDependency {
    RegistryDependency {
        #[serde(rename = "type")]
        pkg_type: String,
        name: String,
        version: String,
    },

    LocalDependency {
        path: String,
    },
}

impl From<&PkgDependency> for ManifestDependency {
    fn from(pkg_dependency: &PkgDependency) -> Self {
        if let Some(path) = &pkg_dependency.path {
            ManifestDependency::LocalDependency { path: path.clone() }
        } else {
            ManifestDependency::RegistryDependency {
                pkg_type: pkg_dependency.type_and_name.pkg_type.to_string(),
                name: pkg_dependency.type_and_name.name.clone(),
                version: pkg_dependency.version_req.to_string(),
            }
        }
    }
}

impl From<&PkgInfo> for ManifestDependency {
    fn from(pkg_info: &PkgInfo) -> Self {
        if pkg_info.is_local_dependency {
            ManifestDependency::LocalDependency {
                path: {
                    assert!(
                        pkg_info.local_dependency_path.is_some(),
                        "Should not happen."
                    );

                    pkg_info.local_dependency_path.as_ref().unwrap().clone()
                },
            }
        } else {
            ManifestDependency::RegistryDependency {
                pkg_type: pkg_info
                    .basic_info
                    .type_and_name
                    .pkg_type
                    .to_string(),
                name: pkg_info.basic_info.type_and_name.name.clone(),

                // Use the package's version as the declared dependency version.
                version: pkg_info.basic_info.version.to_string(),
            }
        }
    }
}
