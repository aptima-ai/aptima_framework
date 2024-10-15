//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
use std::collections::HashMap;

use anyhow::Result;

use crate::pkg_info::{graph::Graph, pkg_type::PkgType, PkgInfo};

impl Graph {
    pub fn check_if_nodes_have_installed(
        &self,
        existed_pkgs_of_all_apps: &HashMap<String, Vec<PkgInfo>>,
    ) -> Result<()> {
        // app_uri, node_type, node_addon_name
        let mut not_installed_pkgs: Vec<(String, PkgType, String)> = Vec::new();

        for node in &self.nodes {
            let node_app_uri = node.get_app_uri();

            // Check if the app to which the graph node belongs has been
            // specified.
            if !existed_pkgs_of_all_apps.contains_key(node_app_uri) {
                not_installed_pkgs.push((
                    node_app_uri.to_string(),
                    node.node_type.clone(),
                    node.addon.clone(),
                ));
            }

            let existed_pkgs_of_app =
                existed_pkgs_of_all_apps.get(node_app_uri).unwrap();

            // Check if the graph node exists in the specified app.
            let found = existed_pkgs_of_app.iter().find(|pkg| {
                pkg.pkg_identity.pkg_type == node.node_type
                    && pkg.pkg_identity.name == node.addon
                    && pkg.is_local_installed
            });
            if found.is_none() {
                not_installed_pkgs.push((
                    node_app_uri.to_string(),
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
