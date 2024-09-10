//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
use crate::pkg_info::graph::{
    GraphConnection, GraphDestination, GraphMessageFlow,
};

#[derive(Debug, Clone)]
pub struct PkgConnection {
    pub app: String,
    pub extension_group: String,
    pub extension: String,

    pub cmd: Vec<PkgMessageFlow>,
    pub data: Vec<PkgMessageFlow>,
    pub audio_frame: Vec<PkgMessageFlow>,
    pub video_frame: Vec<PkgMessageFlow>,
}

impl From<GraphConnection> for PkgConnection {
    fn from(manifest_connection: GraphConnection) -> Self {
        PkgConnection {
            app: manifest_connection.app,
            extension_group: manifest_connection.extension_group,
            extension: manifest_connection.extension,

            cmd: match manifest_connection.cmd {
                Some(cmd) => cmd.into_iter().map(|m| m.into()).collect(),
                None => Vec::new(),
            },
            data: match manifest_connection.data {
                Some(data) => data.into_iter().map(|m| m.into()).collect(),
                None => Vec::new(),
            },
            audio_frame: match manifest_connection.audio_frame {
                Some(audio_frame) => {
                    audio_frame.into_iter().map(|m| m.into()).collect()
                }
                None => Vec::new(),
            },
            video_frame: match manifest_connection.video_frame {
                Some(video_frame) => {
                    video_frame.into_iter().map(|m| m.into()).collect()
                }
                None => Vec::new(),
            },
        }
    }
}

#[derive(Debug, Clone)]
pub struct PkgMessageFlow {
    pub name: String,
    pub dest: Vec<PkgDestination>,
}

impl From<GraphMessageFlow> for PkgMessageFlow {
    fn from(manifest_message_flow: GraphMessageFlow) -> Self {
        PkgMessageFlow {
            name: manifest_message_flow.name,
            dest: manifest_message_flow
                .dest
                .into_iter()
                .map(|d| d.into())
                .collect(),
        }
    }
}

#[derive(Debug, Clone)]
pub struct PkgDestination {
    pub app: String,
    pub extension_group: String,
    pub extension: String,
}

impl From<GraphDestination> for PkgDestination {
    fn from(manifest_destination: GraphDestination) -> Self {
        PkgDestination {
            app: manifest_destination.app,
            extension_group: manifest_destination.extension_group,
            extension: manifest_destination.extension,
        }
    }
}
