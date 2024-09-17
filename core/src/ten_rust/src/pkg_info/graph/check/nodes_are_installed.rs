//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
use std::collections::HashMap;

use anyhow::Result;

use crate::pkg_info::{graph::Graph, PkgInfo};

impl Graph {
    pub fn check_if_nodes_have_installed(
        &self,
        all_needed_pkgs: &HashMap<String, Vec<PkgInfo>>,
    ) -> Result<()> {
        // app, node_type, node_addon
        let mut not_installed_pkgs: Vec<(String, String, String)> = Vec::new();

        for node in &self.nodes {
            if !all_needed_pkgs.contains_key(node.app.as_str()) {
                not_installed_pkgs.push((
                    node.app.clone(),
                    node.node_type.clone(),
                    node.addon.clone(),
                ));
            }

            let pkgs_in_app = all_needed_pkgs.get(node.app.as_str()).unwrap();
            let found = pkgs_in_app.iter().find(|pkg| {
                pkg.pkg_identity.pkg_type.to_string() == node.node_type
                    && pkg.pkg_identity.name == node.addon
                    && pkg.is_local_installed
            });
            if found.is_none() {
                not_installed_pkgs.push((
                    node.app.clone(),
                    node.node_type.clone(),
                    node.addon.clone(),
                ));
            }
        }

        if !not_installed_pkgs.is_empty() {
            return Err(anyhow::anyhow!(
                "The following packages are declared in nodes but not installed: {:?}.",
                not_installed_pkgs
            ));
        }

        Ok(())
    }
}
