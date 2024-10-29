//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
use crate::dev_server::graphs::connections::{
    DevServerConnection, DevServerDestination, DevServerMessageFlow,
};
use ten_rust::pkg_info::graph::{
    GraphConnection, GraphDestination, GraphMessageFlow,
};

impl From<DevServerConnection> for GraphConnection {
    fn from(dev_server_connection: DevServerConnection) -> Self {
        GraphConnection {
            app: Some(dev_server_connection.app),
            extension_group: dev_server_connection.extension_group,
            extension: dev_server_connection.extension,

            cmd: dev_server_connection
                .cmd
                .map(get_property_msg_flow_from_dev_server),
            data: dev_server_connection
                .data
                .map(get_property_msg_flow_from_dev_server),
            audio_frame: dev_server_connection
                .audio_frame
                .map(get_property_msg_flow_from_dev_server),
            video_frame: dev_server_connection
                .video_frame
                .map(get_property_msg_flow_from_dev_server),
        }
    }
}

fn get_property_msg_flow_from_dev_server(
    msg_flow: Vec<DevServerMessageFlow>,
) -> Vec<GraphMessageFlow> {
    msg_flow.into_iter().map(|v| v.into()).collect()
}

impl From<DevServerMessageFlow> for GraphMessageFlow {
    fn from(dev_server_msg_flow: DevServerMessageFlow) -> Self {
        GraphMessageFlow {
            name: dev_server_msg_flow.name,
            dest: dev_server_msg_flow
                .dest
                .into_iter()
                .map(|d| d.into())
                .collect(),
        }
    }
}

impl From<DevServerDestination> for GraphDestination {
    fn from(dev_server_destination: DevServerDestination) -> Self {
        GraphDestination {
            app: Some(dev_server_destination.app),
            extension_group: dev_server_destination.extension_group,
            extension: dev_server_destination.extension,
            msg_conversion: dev_server_destination.msg_conversion,
        }
    }
}
