//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
use ten_rust::pkg_info::{graph::GraphNode, pkg_type::PkgType};

use crate::dev_server::graphs::nodes::DevServerExtension;

impl From<DevServerExtension> for GraphNode {
    fn from(dev_server_extension: DevServerExtension) -> Self {
        GraphNode {
            node_type: PkgType::Extension,
            name: dev_server_extension.name,
            addon: dev_server_extension.addon,
            extension_group: Some(dev_server_extension.extension_group.clone()),
            app: Some(dev_server_extension.app),
            property: dev_server_extension.property,
        }
    }
}
