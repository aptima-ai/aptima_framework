//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
use std::collections::HashMap;

use anyhow::{Ok, Result};

use crate::pkg_info::graph::{Graph, GraphConnection, GraphMessageFlow};

impl Graph {
    fn check_if_message_names_are_duplicated(&self) -> Result<()> {
        let mut errors: Vec<String> = vec![];
        for (conn_idx, connection) in
            self.connections.as_ref().unwrap().iter().enumerate()
        {
            if let Some(errs) =
                self.check_if_message_name_duplicated_in_connection(connection)
            {
                errors.push(format!("- connection[{}]:", conn_idx));
                for err in errs {
                    errors.push(format!("  {}", err));
                }
            }
        }

        if errors.is_empty() {
            Ok(())
        } else {
            Err(anyhow::anyhow!("{}", errors.join("\n")))
        }
    }

    fn check_if_message_name_duplicated_group_by_msg_type(
        &self,
        flows: &[GraphMessageFlow],
    ) -> Vec<String> {
        let mut errors: Vec<String> = vec![];

        let mut msg_names: HashMap<String, usize> = HashMap::new();
        for (flow_idx, flow) in flows.iter().enumerate() {
            if let Some(idx) = msg_names.get(&flow.name) {
                errors.push(format!(
                    "'{}' is defined in flow[{}] and flow[{}].",
                    flow.name, idx, flow_idx
                ));
            } else {
                msg_names.insert(flow.name.clone(), flow_idx);
            }
        }

        errors
    }

    fn check_if_message_name_duplicated_in_connection(
        &self,
        connection: &GraphConnection,
    ) -> Option<Vec<String>> {
        let mut errors: Vec<String> = vec![];

        if let Some(cmd) = &connection.cmd {
            let errs =
                self.check_if_message_name_duplicated_group_by_msg_type(cmd);
            if !errs.is_empty() {
                errors.push(
                    "- Merge the following cmd into one section:".to_string(),
                );
                for err in errs {
                    errors.push(format!("  {}", err));
                }
            }
        }

        if let Some(data) = &connection.data {
            let errs =
                self.check_if_message_name_duplicated_group_by_msg_type(data);
            if !errs.is_empty() {
                errors.push(
                    "- Merge the following data into one section:".to_string(),
                );
                for err in errs {
                    errors.push(format!("  {}", err));
                }
            }
        }

        if let Some(audio_frame) = &connection.audio_frame {
            let errs = self.check_if_message_name_duplicated_group_by_msg_type(
                audio_frame,
            );
            if !errs.is_empty() {
                errors.push(
                    "- Merge the following auto_frame into one section:"
                        .to_string(),
                );
                for err in errs {
                    errors.push(format!("  {}", err));
                }
            }
        }

        if let Some(video_frame) = &connection.video_frame {
            let errs = self.check_if_message_name_duplicated_group_by_msg_type(
                video_frame,
            );
            if !errs.is_empty() {
                errors.push(
                    "- Merge the following video_frame into one section:"
                        .to_string(),
                );
                for err in errs {
                    errors.push(format!("  {}", err));
                }
            }
        }

        if errors.is_empty() {
            None
        } else {
            Some(errors)
        }
    }

    pub fn check_message_names(&self) -> Result<()> {
        if self.connections.is_none() {
            return Ok(());
        }

        self.check_if_message_names_are_duplicated()?;

        Ok(())
    }
}
