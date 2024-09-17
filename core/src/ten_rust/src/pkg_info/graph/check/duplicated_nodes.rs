//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
use anyhow::Result;

use crate::pkg_info::graph::Graph;

impl Graph {
    pub fn check_if_nodes_duplicated(&self) -> Result<()> {
        let mut groups_used_in_extensions: Vec<String> = Vec::new();
        let mut all_extensions: Vec<String> = Vec::new();
        let mut all_extension_groups: Vec<String> = Vec::new();

        for (node_idx, node) in self.nodes.iter().enumerate() {
            match node.node_type.as_str() {
                "extension" => {
                    let unique_ext_name = format!(
                        "{}:{}:{}",
                        node.app.as_str(),
                        node.extension_group.as_ref().unwrap(),
                        node.name
                    );

                    if all_extensions.contains(&unique_ext_name) {
                        return Err(anyhow::anyhow!(
                            "Duplicated extension was found in nodes[{}], addon: {}, name: {}.",
                            node_idx, node.addon, node.name
                        ));
                    }

                    all_extensions.push(unique_ext_name.clone());

                    let ext_group = format!(
                        "{}:{}",
                        node.app.as_str(),
                        node.extension_group.as_ref().unwrap()
                    );

                    if !groups_used_in_extensions.contains(&ext_group) {
                        groups_used_in_extensions.push(ext_group);
                    }
                }

                "extension_group" => {
                    let unique_ext_group_name =
                        format!("{}:{}", node.app.as_str(), node.name);

                    if all_extension_groups.contains(&unique_ext_group_name) {
                        return Err(anyhow::anyhow!(
                            "Duplicated extension_group was found in nodes[{}], addon: {}, name: {}.",
                            node_idx, node.addon, node.name
                        ));
                    }

                    all_extension_groups.push(unique_ext_group_name);
                }

                _ => {}
            }
        }

        if all_extensions.is_empty() {
            return Err(anyhow::anyhow!(
                "No extension node is defined in graph."
            ));
        }

        let unused_ext_groups = all_extension_groups
            .iter()
            .filter(|ext_group| !groups_used_in_extensions.contains(ext_group))
            .collect::<Vec<&String>>();

        if !unused_ext_groups.is_empty() {
            return Err(anyhow::anyhow!(
                "The following extension group nodes are not used by any extension nodes: {:?}.",
                unused_ext_groups
            ));
        }

        Ok(())
    }
}
