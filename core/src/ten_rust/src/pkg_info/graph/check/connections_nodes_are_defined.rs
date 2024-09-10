//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
use anyhow::Result;

use crate::pkg_info::graph::{Graph, GraphMessageFlow};

impl Graph {
    fn check_if_dest_of_connection_are_defined_in_nodes(
        all_extensions: &[String],
        flows: &[GraphMessageFlow],
        conn_idx: usize,
        msg_type: &str,
    ) -> Result<()> {
        for (flow_idx, flow) in flows.iter().enumerate() {
            for dest in &flow.dest {
                let dest_extension = format!(
                    "{}:{}:{}",
                    dest.app, dest.extension_group, dest.extension
                );

                if !all_extensions.contains(&dest_extension) {
                    return Err(anyhow::anyhow!(
                        "The extension declared in connections[{}].{}[{}] is not defined in nodes, extension_group: {}, extension: {}.",
                        conn_idx,
                        msg_type,
                        flow_idx,
                        dest.extension_group,
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
            if node.node_type.as_str() == "extension" {
                let unique_ext_name = format!(
                    "{}:{}:{}",
                    node.app.as_str(),
                    node.extension_group.as_ref().unwrap(),
                    node.name
                );
                all_extensions.push(unique_ext_name);
            }
        }

        let connections = self.connections.as_ref().unwrap();
        for (conn_idx, connection) in connections.iter().enumerate() {
            let src_extension = format!(
                "{}:{}:{}",
                connection.app,
                connection.extension_group,
                connection.extension
            );
            if !all_extensions.contains(&src_extension) {
                return Err(anyhow::anyhow!(
                    "The extension declared in connections[{}] is not defined in nodes, extension_group: {}, extension: {}.",
                    conn_idx,
                    connection.extension_group,
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
