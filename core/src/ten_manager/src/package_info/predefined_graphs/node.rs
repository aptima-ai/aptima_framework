//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//

use crate::dev_server::graphs::nodes::DevServerExtension;
use ten_rust::pkg_info::{pkg_type::PkgType, predefined_graphs::node::PkgNode};

impl From<DevServerExtension> for PkgNode {
    fn from(dev_server_extension: DevServerExtension) -> Self {
        PkgNode {
            node_type: PkgType::Extension,
            name: dev_server_extension.name,
            addon: dev_server_extension.addon,
            extension_group: Some(dev_server_extension.extension_group.clone()),
            app: dev_server_extension.app,
            property: dev_server_extension.property,
            pkg_info: None,
        }
    }
}
