//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
use anyhow::Result;
use semver::VersionReq;

/// Used to parse a pattern like `aaa@3.0.0` and return `aaa` and `3.0.0`.
pub fn parse_pkg_name_version_req(
    pkg_name_version: &str,
) -> Result<(String, VersionReq)> {
    let parts: Vec<&str> = pkg_name_version.split('@').collect();
    if parts.len() == 2 {
        Ok((parts[0].to_string(), VersionReq::parse(parts[1])?))
    } else {
        Ok((pkg_name_version.to_string(), VersionReq::STAR))
    }
}
