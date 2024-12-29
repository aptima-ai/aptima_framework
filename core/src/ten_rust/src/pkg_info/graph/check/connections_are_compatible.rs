//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
use std::collections::HashMap;

use anyhow::Result;

use crate::{
    pkg_info::{
        graph::{Graph, GraphConnection, GraphDestination},
        message::{MsgDirection, MsgType},
        pkg_type::PkgType,
        PkgInfo,
    },
    schema::{
        store::{
            are_cmd_schemas_compatible, are_ten_schemas_compatible, CmdSchema,
        },
        TenSchema,
    },
};

impl Graph {
    fn get_addon_name_of_extension(&self, app: &str, extension: &str) -> &str {
        self.nodes
            .iter()
            .find_map(|node| {
                if node.type_and_name.pkg_type == PkgType::Extension
                    && node.type_and_name.name.as_str() == extension
                    && node.get_app_uri() == app
                {
                    Some(node.addon.as_str())
                } else {
                    None
                }
            })
            .unwrap_or_else(|| {
                panic!(
                    "Extension '{}' is not found in nodes, should not happen.",
                    extension
                )
            })
    }

    fn check_msg_flow_compatible(
        &self,
        existed_pkgs_of_all_apps: &HashMap<String, Vec<PkgInfo>>,
        msg_name: &str,
        msg_type: &MsgType,
        src_msg_schema: Option<&TenSchema>,
        dests: &[GraphDestination],
    ) -> Result<()> {
        let mut errors: Vec<String> = Vec::new();
        for dest in dests {
            let dest_addon = self.get_addon_name_of_extension(
                dest.get_app_uri(),
                dest.extension.as_str(),
            );
            let dest_msg_schema = find_msg_schema_from_all_pkgs_info(
                existed_pkgs_of_all_apps,
                dest.get_app_uri(),
                dest_addon,
                msg_name,
                msg_type,
                MsgDirection::In,
            );

            if let Err(e) =
                are_ten_schemas_compatible(src_msg_schema, dest_msg_schema)
            {
                errors.push(format!(
                    "Schema incompatible to [extension: {}], {}",
                    dest.extension, e
                ));
            }
        }

        if errors.is_empty() {
            Ok(())
        } else {
            Err(anyhow::anyhow!("{}", errors.join("\n")))
        }
    }

    fn check_cmd_flow_compatible(
        &self,
        existed_pkgs_of_all_apps: &HashMap<String, Vec<PkgInfo>>,
        cmd_name: &str,
        src_cmd_schema: Option<&CmdSchema>,
        dests: &[GraphDestination],
        skip_if_app_not_exist: bool,
    ) -> Result<()> {
        let mut errors: Vec<String> = Vec::new();
        for dest in dests {
            let dest_addon = self.get_addon_name_of_extension(
                dest.get_app_uri(),
                dest.extension.as_str(),
            );

            let existed_pkgs_of_dest_app =
                existed_pkgs_of_all_apps.get(dest.get_app_uri());
            if existed_pkgs_of_dest_app.is_none() {
                if skip_if_app_not_exist {
                    continue;
                } else {
                    return Err(anyhow::anyhow!(
                        "App [{}] is not found in the pkgs map, should not happen.",
                        dest.get_app_uri()
                    ));
                }
            }

            let dest_cmd_schema = find_cmd_schema_from_all_pkgs_info(
                existed_pkgs_of_dest_app.unwrap(),
                dest_addon,
                cmd_name,
                MsgDirection::In,
            );

            if let Err(e) =
                are_cmd_schemas_compatible(src_cmd_schema, dest_cmd_schema)
            {
                errors.push(format!(
                    "Schema incompatible to [extension: {}], {}",
                    dest.extension, e
                ));
            }
        }

        if errors.is_empty() {
            Ok(())
        } else {
            Err(anyhow::anyhow!("{}", errors.join("\n")))
        }
    }

    fn check_connection_compatible(
        &self,
        existed_pkgs_of_all_apps: &HashMap<String, Vec<PkgInfo>>,
        connection: &GraphConnection,
        skip_if_app_not_exist: bool,
    ) -> Result<()> {
        let mut errors: Vec<String> = Vec::new();
        if let Some(cmd_flows) = &connection.cmd {
            for (flow_idx, flow) in cmd_flows.iter().enumerate() {
                let src_addon = self.get_addon_name_of_extension(
                    connection.get_app_uri(),
                    connection.extension.as_str(),
                );

                let existed_pkgs_of_src_app =
                    existed_pkgs_of_all_apps.get(connection.get_app_uri());
                if existed_pkgs_of_src_app.is_none() {
                    if skip_if_app_not_exist {
                        continue;
                    } else {
                        return Err(anyhow::anyhow!("App [{}] is not found in the pkgs map, should not happen.", connection.get_app_uri()));
                    }
                }

                let src_cmd_schema = find_cmd_schema_from_all_pkgs_info(
                    existed_pkgs_of_src_app.unwrap(),
                    src_addon,
                    flow.name.as_str(),
                    MsgDirection::Out,
                );

                if let Err(e) = self.check_cmd_flow_compatible(
                    existed_pkgs_of_all_apps,
                    flow.name.as_str(),
                    src_cmd_schema,
                    &flow.dest,
                    skip_if_app_not_exist,
                ) {
                    errors.push(format!("- cmd[{}]:  {}", flow_idx, e));
                }
            }
        }

        if let Some(data_flows) = &connection.data {
            for (flow_idx, flow) in data_flows.iter().enumerate() {
                let src_addon = self.get_addon_name_of_extension(
                    connection.get_app_uri(),
                    connection.extension.as_str(),
                );
                let src_msg_schema = find_msg_schema_from_all_pkgs_info(
                    existed_pkgs_of_all_apps,
                    connection.get_app_uri(),
                    src_addon,
                    flow.name.as_str(),
                    &MsgType::Data,
                    MsgDirection::Out,
                );

                if let Err(e) = self.check_msg_flow_compatible(
                    existed_pkgs_of_all_apps,
                    flow.name.as_str(),
                    &MsgType::Data,
                    src_msg_schema,
                    &flow.dest,
                ) {
                    errors.push(format!("- data[{}]:  {}", flow_idx, e));
                }
            }
        }

        if let Some(video_frame_flows) = &connection.video_frame {
            for (flow_idx, flow) in video_frame_flows.iter().enumerate() {
                let src_addon = self.get_addon_name_of_extension(
                    connection.get_app_uri(),
                    connection.extension.as_str(),
                );
                let src_msg_schema = find_msg_schema_from_all_pkgs_info(
                    existed_pkgs_of_all_apps,
                    connection.get_app_uri(),
                    src_addon,
                    flow.name.as_str(),
                    &MsgType::VideoFrame,
                    MsgDirection::Out,
                );

                if let Err(e) = self.check_msg_flow_compatible(
                    existed_pkgs_of_all_apps,
                    flow.name.as_str(),
                    &MsgType::VideoFrame,
                    src_msg_schema,
                    &flow.dest,
                ) {
                    errors.push(format!("- video_frame[{}]:  {}", flow_idx, e));
                }
            }
        }

        if let Some(audio_frame_flows) = &connection.audio_frame {
            for (flow_idx, flow) in audio_frame_flows.iter().enumerate() {
                let src_addon = self.get_addon_name_of_extension(
                    connection.get_app_uri(),
                    connection.extension.as_str(),
                );
                let src_msg_schema = find_msg_schema_from_all_pkgs_info(
                    existed_pkgs_of_all_apps,
                    connection.get_app_uri(),
                    src_addon,
                    flow.name.as_str(),
                    &MsgType::AudioFrame,
                    MsgDirection::Out,
                );

                if let Err(e) = self.check_msg_flow_compatible(
                    existed_pkgs_of_all_apps,
                    flow.name.as_str(),
                    &MsgType::AudioFrame,
                    src_msg_schema,
                    &flow.dest,
                ) {
                    errors.push(format!("- audio_frame[{}]:  {}", flow_idx, e));
                }
            }
        }

        if errors.is_empty() {
            Ok(())
        } else {
            Err(anyhow::anyhow!("{}", errors.join("\n  ")))
        }
    }

    pub fn check_connections_compatible(
        &self,
        existed_pkgs_of_all_apps: &HashMap<String, Vec<PkgInfo>>,
        skip_if_app_not_exist: bool,
    ) -> Result<()> {
        if self.connections.is_none() {
            return Ok(());
        }

        let mut errors: Vec<String> = Vec::new();

        let connections = self.connections.as_ref().unwrap();
        for (conn_idx, connection) in connections.iter().enumerate() {
            if let Err(e) = self.check_connection_compatible(
                existed_pkgs_of_all_apps,
                connection,
                skip_if_app_not_exist,
            ) {
                errors.push(format!("- connections[{}]: \n  {}", conn_idx, e));
            }
        }

        if errors.is_empty() {
            Ok(())
        } else {
            Err(anyhow::anyhow!("{}", errors.join("\n")))
        }
    }
}

fn find_cmd_schema_from_all_pkgs_info<'a>(
    existed_pkgs_of_app: &'a [PkgInfo],
    addon: &str,
    cmd_name: &str,
    direction: MsgDirection,
) -> Option<&'a CmdSchema> {
    let addon_pkg = existed_pkgs_of_app
        .iter()
        .find(|pkg| {
            pkg.basic_info.type_and_name.pkg_type == PkgType::Extension
                && pkg.basic_info.type_and_name.name == addon
        })
        .unwrap_or_else(|| {
            panic!("should not happen.");
        });

    let schema_store = addon_pkg.schema_store.as_ref()?;

    match direction {
        MsgDirection::In => schema_store.cmd_in.get(cmd_name),
        MsgDirection::Out => schema_store.cmd_out.get(cmd_name),
    }
}

fn find_msg_schema_from_all_pkgs_info<'a>(
    existed_pkgs_of_all_apps: &'a HashMap<String, Vec<PkgInfo>>,
    app: &str,
    addon: &str,
    msg_name: &str,
    msg_type: &MsgType,
    direction: MsgDirection,
) -> Option<&'a TenSchema> {
    let existed_pkgs_of_app =
        existed_pkgs_of_all_apps.get(app).unwrap_or_else(|| {
            panic!("should not happen.");
        });

    let addon_pkg = existed_pkgs_of_app
        .iter()
        .find(|pkg| {
            pkg.basic_info.type_and_name.pkg_type == PkgType::Extension
                && pkg.basic_info.type_and_name.name == addon
        })
        .unwrap_or_else(|| {
            panic!("should not happen.");
        });

    let schema_store = addon_pkg.schema_store.as_ref()?;

    match msg_type {
        MsgType::Data => match direction {
            MsgDirection::In => schema_store.data_in.get(msg_name),
            MsgDirection::Out => schema_store.data_out.get(msg_name),
        },
        MsgType::AudioFrame => match direction {
            MsgDirection::In => schema_store.audio_frame_in.get(msg_name),
            MsgDirection::Out => schema_store.audio_frame_out.get(msg_name),
        },
        MsgType::VideoFrame => match direction {
            MsgDirection::In => schema_store.video_frame_in.get(msg_name),
            MsgDirection::Out => schema_store.video_frame_out.get(msg_name),
        },
        _ => panic!("Unsupported message type: {}", msg_type),
    }
}
