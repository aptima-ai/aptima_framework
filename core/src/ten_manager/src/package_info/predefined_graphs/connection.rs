//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
use crate::dev_server::graphs::connections::{
    DevServerConnection, DevServerDestination, DevServerMessageFlow,
};
use ten_rust::pkg_info::predefined_graphs::connection::{
    PkgConnection, PkgDestination, PkgMessageFlow,
};

impl From<DevServerConnection> for PkgConnection {
    fn from(dev_server_connection: DevServerConnection) -> Self {
        PkgConnection {
            app: dev_server_connection.app,
            extension_group: dev_server_connection.extension_group,
            extension: dev_server_connection.extension,

            cmd: match dev_server_connection.cmd {
                Some(cmd) => get_pkg_msg_flow_from_dev_server(cmd),
                None => Vec::new(),
            },
            data: match dev_server_connection.data {
                Some(data) => get_pkg_msg_flow_from_dev_server(data),
                None => Vec::new(),
            },
            audio_frame: match dev_server_connection.audio_frame {
                Some(audio_frame) => {
                    get_pkg_msg_flow_from_dev_server(audio_frame)
                }
                None => Vec::new(),
            },
            video_frame: match dev_server_connection.video_frame {
                Some(video_frame) => {
                    get_pkg_msg_flow_from_dev_server(video_frame)
                }
                None => Vec::new(),
            },
        }
    }
}

fn get_pkg_msg_flow_from_dev_server(
    msg_flow: Vec<DevServerMessageFlow>,
) -> Vec<PkgMessageFlow> {
    msg_flow.into_iter().map(|v| v.into()).collect()
}

impl From<DevServerMessageFlow> for PkgMessageFlow {
    fn from(dev_server_msg_flow: DevServerMessageFlow) -> Self {
        PkgMessageFlow {
            name: dev_server_msg_flow.name,
            dest: dev_server_msg_flow
                .dest
                .into_iter()
                .map(|d| d.into())
                .collect(),
        }
    }
}

impl From<DevServerDestination> for PkgDestination {
    fn from(dev_server_destination: DevServerDestination) -> Self {
        PkgDestination {
            app: dev_server_destination.app,
            extension_group: dev_server_destination.extension_group,
            extension: dev_server_destination.extension,
        }
    }
}
