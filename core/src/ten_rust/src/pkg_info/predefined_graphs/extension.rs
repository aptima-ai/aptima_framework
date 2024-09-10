//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
use anyhow::Result;

use super::pkg_predefined_graphs_find;
use crate::{
    pkg_info::{
        api::{PkgApiCmdLike, PkgApiDataLike},
        message::{MsgDirection, MsgType},
        pkg_type::PkgType,
        predefined_graphs::node::PkgNode,
        PkgInfo,
    },
    schema::{
        store::{
            are_cmd_schemas_compatible, are_ten_schemas_compatible, CmdSchema,
        },
        TenSchema,
    },
};

pub fn get_extension_nodes_in_graph(
    graph_name: &String,
    all_pkgs: &[PkgInfo],
) -> Result<Vec<PkgNode>> {
    if let Some(app_pkg) = all_pkgs
        .iter()
        .find(|pkg| pkg.pkg_identity.pkg_type == PkgType::App)
    {
        if app_pkg.property.is_none() {
            return Err(anyhow::anyhow!("'property.json' file not found."));
        }

        // Look for the graph by name in the predefined_graphs of the app
        // package.
        if let Some(graph) =
            pkg_predefined_graphs_find(&app_pkg.predefined_graphs, |graph| {
                graph.name == *graph_name
            })
        {
            // Collect all extension nodes from the graph.
            let extension_nodes: Vec<_> = graph
                .nodes
                .iter()
                .filter(|node| node.node_type == PkgType::Extension)
                .collect();

            Ok(extension_nodes.into_iter().cloned().collect())
        } else {
            Err(anyhow::anyhow!(
                format!("Graph {} not found", graph_name).to_string(),
            ))
        }
    } else {
        Err(anyhow::anyhow!("Package information is missing"))
    }
}

pub fn get_extension_nodes_pkg_info(
    extensions: &mut [PkgNode],
    all_pkgs: &[PkgInfo],
) -> Result<()> {
    for extension in extensions.iter_mut() {
        if let Some(pkg_info) = all_pkgs.iter().find(|pkg| {
            pkg.pkg_identity.pkg_type == PkgType::Extension
                && pkg.pkg_identity.name == extension.addon
        }) {
            extension.pkg_info = Some(pkg_info.clone());
            continue;
        }

        return Err(anyhow::anyhow!(
            "the addon '{}' used to instantiate extension '{}' is not found, check your addons in ten_packages/extension.",
            extension.addon,
            extension.name
        ));
    }
    Ok(())
}

pub fn get_extension<'a>(
    extensions: &'a [PkgNode],
    app: &String,
    extension_group: &String,
    extension: &String,
) -> Result<&'a PkgNode> {
    extensions
        .iter()
        .find(|ext| {
            ext.node_type == PkgType::Extension
                && &ext.app == app
                && ext.extension_group.clone().unwrap_or("".to_string())
                    == *extension_group
                && &ext.name == extension
        })
        .ok_or_else(|| anyhow::anyhow!("Extension not found"))
}

pub fn get_cmd_message<'a>(
    extension: &'a PkgNode,
    cmd_name: &String,
    direction: &MsgDirection,
) -> Result<&'a PkgApiCmdLike> {
    extension
        .pkg_info
        .as_ref()
        .and_then(|pkg_info| {
            pkg_info.api.as_ref().and_then(|api| match direction {
                MsgDirection::In => {
                    api.cmd_in.iter().find(|cmd| &cmd.name == cmd_name)
                }
                MsgDirection::Out => {
                    api.cmd_out.iter().find(|cmd| &cmd.name == cmd_name)
                }
            })
        })
        .ok_or_else(|| anyhow::anyhow!("Command not found"))
}

pub fn get_data_like_message<'a>(
    extension: &'a PkgNode,
    msg_type: &MsgType,
    msg_name: &String,
    direction: &MsgDirection,
) -> Result<&'a PkgApiDataLike> {
    extension
        .pkg_info
        .as_ref()
        .and_then(|pkg_info| {
            pkg_info.api.as_ref().and_then(|api| match direction {
                MsgDirection::In => match msg_type {
                    MsgType::Data => {
                        api.data_in.iter().find(|msg| &msg.name == msg_name)
                    }
                    MsgType::AudioFrame => api
                        .audio_frame_in
                        .iter()
                        .find(|msg| &msg.name == msg_name),
                    MsgType::VideoFrame => api
                        .video_frame_in
                        .iter()
                        .find(|msg| &msg.name == msg_name),
                    _ => {
                        panic!("Unsupported message type: {}", msg_type)
                    }
                },
                MsgDirection::Out => match msg_type {
                    MsgType::Data => {
                        api.data_out.iter().find(|msg| &msg.name == msg_name)
                    }
                    MsgType::AudioFrame => api
                        .audio_frame_out
                        .iter()
                        .find(|msg| &msg.name == msg_name),
                    MsgType::VideoFrame => api
                        .video_frame_out
                        .iter()
                        .find(|msg| &msg.name == msg_name),
                    _ => {
                        panic!("Unsupported message type: {}", msg_type)
                    }
                },
            })
        })
        .ok_or_else(|| anyhow::anyhow!("Command not found"))
}

pub struct CompatibleExtensionAndMsg<'a> {
    pub extension: &'a PkgNode,
    pub msg_type: MsgType,
    pub msg_direction: MsgDirection,
    pub msg_name: String,
}

pub fn get_compatible_cmd_extension<'a>(
    extensions: &'a [PkgNode],
    desired_msg_dir: &MsgDirection,
    pivot: Option<&CmdSchema>,
    cmd_name: &str,
) -> Result<Vec<CompatibleExtensionAndMsg<'a>>> {
    let mut result = Vec::new();

    for ext in extensions {
        let target_cmd_schema = ext.pkg_info.as_ref().and_then(|pkg| {
            pkg.schema_store.as_ref().and_then(|schema_store| {
                match desired_msg_dir {
                    MsgDirection::In => schema_store.cmd_in.get(cmd_name),
                    MsgDirection::Out => schema_store.cmd_out.get(cmd_name),
                }
            })
        });

        let compatible = match desired_msg_dir {
            MsgDirection::In => {
                are_cmd_schemas_compatible(pivot, target_cmd_schema)
            }
            MsgDirection::Out => {
                are_cmd_schemas_compatible(target_cmd_schema, pivot)
            }
        };

        if compatible.is_ok() {
            result.push(CompatibleExtensionAndMsg {
                extension: ext,
                msg_type: MsgType::Cmd,
                msg_name: cmd_name.to_string(),
                msg_direction: desired_msg_dir.clone(),
            });
        }
    }

    Ok(result)
}

pub fn get_compatible_data_like_msg_extension<'a>(
    extensions: &'a [PkgNode],
    desired_msg_dir: &MsgDirection,
    pivot: Option<&TenSchema>,
    msg_type: &MsgType,
    msg_name: String,
) -> Result<Vec<CompatibleExtensionAndMsg<'a>>> {
    let mut result = Vec::new();

    for ext in extensions {
        if ext.pkg_info.is_none() {
            continue;
        }

        let pkg_info = ext.pkg_info.as_ref().unwrap();
        let target_msg_schema =
            pkg_info.schema_store.as_ref().and_then(|schema_store| {
                let msg_name = msg_name.as_str();
                match msg_type {
                    MsgType::Data => match desired_msg_dir {
                        MsgDirection::In => schema_store.data_in.get(msg_name),
                        MsgDirection::Out => {
                            schema_store.data_out.get(msg_name)
                        }
                    },
                    MsgType::AudioFrame => match desired_msg_dir {
                        MsgDirection::In => {
                            schema_store.audio_frame_in.get(msg_name)
                        }
                        MsgDirection::Out => {
                            schema_store.audio_frame_out.get(msg_name)
                        }
                    },
                    MsgType::VideoFrame => match desired_msg_dir {
                        MsgDirection::In => {
                            schema_store.video_frame_in.get(msg_name)
                        }
                        MsgDirection::Out => {
                            schema_store.video_frame_out.get(msg_name)
                        }
                    },
                    _ => {
                        panic!("Unsupported message type: {}", msg_type)
                    }
                }
            });

        let compatible = match desired_msg_dir {
            MsgDirection::In => {
                are_ten_schemas_compatible(pivot, target_msg_schema)
            }
            MsgDirection::Out => {
                are_ten_schemas_compatible(target_msg_schema, pivot)
            }
        };

        if compatible.is_ok() {
            result.push(CompatibleExtensionAndMsg {
                extension: ext,
                msg_type: msg_type.clone(),
                msg_name: msg_name.to_string(),
                msg_direction: desired_msg_dir.clone(),
            });
        }
    }

    Ok(result)
}
