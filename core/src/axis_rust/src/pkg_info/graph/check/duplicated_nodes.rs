//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
use anyhow::Result;

use crate::pkg_info::{graph::Graph, pkg_type::PkgType};

impl Graph {
    pub fn check_if_nodes_duplicated(&self) -> Result<()> {
        let mut all_extensions: Vec<String> = Vec::new();

        for (node_idx, node) in self.nodes.iter().enumerate() {
            if node.type_and_name.pkg_type == PkgType::Extension {
                let unique_ext_name = format!(
                    "{}:{}",
                    node.get_app_uri(),
                    node.type_and_name.name
                );

                if all_extensions.contains(&unique_ext_name) {
                    return Err(anyhow::anyhow!(
                        "Duplicated extension was found in nodes[{}], addon: {}, name: {}.",
                        node_idx, node.addon, node.type_and_name.name
                    ));
                }

                all_extensions.push(unique_ext_name.clone());
            }
        }

        if all_extensions.is_empty() {
            return Err(anyhow::anyhow!(
                "No extension node is defined in graph."
            ));
        }

        Ok(())
    }
}
