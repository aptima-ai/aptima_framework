//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
use axis_rust::pkg_info::graph::{
    GraphConnection, GraphDestination, GraphMessageFlow,
};

use crate::designer::graphs::connections::{
    DesignerConnection, DesignerDestination, DesignerMessageFlow,
};

impl From<DesignerConnection> for GraphConnection {
    fn from(designer_connection: DesignerConnection) -> Self {
        GraphConnection {
            app: Some(designer_connection.app),
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
    msg_flow: Vec<DesignerMessageFlow>,
) -> Vec<GraphMessageFlow> {
    msg_flow.into_iter().map(|v| v.into()).collect()
}

impl From<DesignerMessageFlow> for GraphMessageFlow {
    fn from(designer_msg_flow: DesignerMessageFlow) -> Self {
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

impl From<DesignerDestination> for GraphDestination {
    fn from(designer_destination: DesignerDestination) -> Self {
        GraphDestination {
            app: Some(designer_destination.app),
            extension: designer_destination.extension,
            msg_conversion: designer_destination.msg_conversion,
        }
    }
}
