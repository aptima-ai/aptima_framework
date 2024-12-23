//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
use anyhow::Result;

use crate::pkg_info::{
    graph::{Graph, GraphMessageFlow},
    pkg_type::PkgType,
};

impl Graph {
    fn check_if_dest_of_connection_are_defined_in_nodes(
        all_extensions: &[String],
        flows: &[GraphMessageFlow],
        conn_idx: usize,
        msg_type: &str,
    ) -> Result<()> {
        for (flow_idx, flow) in flows.iter().enumerate() {
            for dest in &flow.dest {
                let dest_extension =
                    format!("{}:{}", dest.get_app_uri(), dest.extension);

                if !all_extensions.contains(&dest_extension) {
                    return Err(anyhow::anyhow!(
                        "The extension declared in connections[{}].{}[{}] is not defined in nodes, extension: {}.",
                        conn_idx,
                        msg_type,
                        flow_idx,
                        dest.extension
                    ));
                }
            }
        }

        Ok(())
    }

    pub fn check_if_extensions_used_in_connections_have_defined_in_nodes(
        &self,
    ) -> Result<()> {
        if self.connections.is_none() {
            return Ok(());
        }

        let mut all_extensions: Vec<String> = Vec::new();
        for node in &self.nodes {
            if node.type_and_name.pkg_type == PkgType::Extension {
                let unique_ext_name = format!(
                    "{}:{}",
                    node.get_app_uri(),
                    node.type_and_name.name
                );
                all_extensions.push(unique_ext_name);
            }
        }

        let connections = self.connections.as_ref().unwrap();
        for (conn_idx, connection) in connections.iter().enumerate() {
            let src_extension = format!(
                "{}:{}",
                connection.get_app_uri(),
                connection.extension
            );
            if !all_extensions.contains(&src_extension) {
                return Err(anyhow::anyhow!(
                    "The extension declared in connections[{}] is not defined in nodes, extension: {}.",
                    conn_idx,
                    connection.extension
                ));
            }

            if let Some(cmd_flows) = &connection.cmd {
                Graph::check_if_dest_of_connection_are_defined_in_nodes(
                    &all_extensions,
                    cmd_flows,
                    conn_idx,
                    "cmd",
                )?;
            }

            if let Some(data_flows) = &connection.data {
                Graph::check_if_dest_of_connection_are_defined_in_nodes(
                    &all_extensions,
                    data_flows,
                    conn_idx,
                    "data",
                )?;
            }

            if let Some(audio_flows) = &connection.audio_frame {
                Graph::check_if_dest_of_connection_are_defined_in_nodes(
                    &all_extensions,
                    audio_flows,
                    conn_idx,
                    "audio_frame",
                )?;
            }

            if let Some(video_flows) = &connection.video_frame {
                Graph::check_if_dest_of_connection_are_defined_in_nodes(
                    &all_extensions,
                    video_flows,
                    conn_idx,
                    "video_frame",
                )?;
            }
        }

        Ok(())
    }
}
