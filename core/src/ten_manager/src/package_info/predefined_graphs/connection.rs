//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
use ten_rust::pkg_info::graph::{
    GraphConnection, GraphDestination, GraphMessageFlow,
};

use crate::designer::graphs::connections::{
    DevServerConnection, DevServerDestination, DevServerMessageFlow,
};

impl From<DevServerConnection> for GraphConnection {
    fn from(designer_connection: DevServerConnection) -> Self {
        GraphConnection {
            app: Some(designer_connection.app),
            extension_group: designer_connection.extension_group,
            extension: designer_connection.extension,

            cmd: designer_connection
                .cmd
                .map(get_property_msg_flow_from_designer),
            data: designer_connection
                .data
                .map(get_property_msg_flow_from_designer),
            audio_frame: designer_connection
                .audio_frame
                .map(get_property_msg_flow_from_designer),
            video_frame: designer_connection
                .video_frame
                .map(get_property_msg_flow_from_designer),
        }
    }
}

fn get_property_msg_flow_from_designer(
    msg_flow: Vec<DevServerMessageFlow>,
) -> Vec<GraphMessageFlow> {
    msg_flow.into_iter().map(|v| v.into()).collect()
}

impl From<DevServerMessageFlow> for GraphMessageFlow {
    fn from(designer_msg_flow: DevServerMessageFlow) -> Self {
        GraphMessageFlow {
            name: designer_msg_flow.name,
            dest: designer_msg_flow
                .dest
                .into_iter()
                .map(|d| d.into())
                .collect(),
        }
    }
}

impl From<DevServerDestination> for GraphDestination {
    fn from(designer_destination: DevServerDestination) -> Self {
        GraphDestination {
            app: Some(designer_destination.app),
            extension_group: designer_destination.extension_group,
            extension: designer_destination.extension,
            msg_conversion: designer_destination.msg_conversion,
        }
    }
}
