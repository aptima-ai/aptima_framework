//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
use std::{fmt, str::FromStr};

use anyhow::{Error, Result};
use serde::{Deserialize, Serialize};

#[derive(
    Clone, Debug, PartialEq, Eq, PartialOrd, Ord, Hash, Serialize, Deserialize,
)]
pub enum PkgType {
    #[serde(rename = "system")]
    System,

    #[serde(rename = "app")]
    App,

    #[serde(rename = "extension_group")]
    ExtensionGroup,

    #[serde(rename = "extension")]
    Extension,

    #[serde(rename = "protocol")]
    Protocol,
}

impl FromStr for PkgType {
    type Err = anyhow::Error;

    fn from_str(s: &str) -> Result<Self> {
        match s {
            "system" => Ok(PkgType::System),
            "app" => Ok(PkgType::App),
            "extension_group" => Ok(PkgType::ExtensionGroup),
            "extension" => Ok(PkgType::Extension),
            "protocol" => Ok(PkgType::Protocol),
            _ => Err(Error::msg("Failed to parse string to package type")),
        }
    }
}

impl fmt::Display for PkgType {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        match self {
            PkgType::System => write!(f, "system"),
            PkgType::App => write!(f, "app"),
            PkgType::ExtensionGroup => write!(f, "extension_group"),
            PkgType::Extension => write!(f, "extension"),
            PkgType::Protocol => write!(f, "protocol"),
        }
    }
}
