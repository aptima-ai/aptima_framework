//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
use std::collections::HashMap;

use anyhow::{Ok, Result};

use crate::pkg_info::graph::Graph;

impl Graph {
    // The following connection is invalid.
    //
    // "connections": [
    //   {
    //     "extension": "some_ext",
    //     "extension_group": "some_group",
    //     "cmd": [
    //       {
    //         "name": "some_cmd",
    //         "dest": [
    //           {
    //             "extension": "ext_2",
    //             "extension_group": "group_2"
    //           }
    //         ]
    //       }
    //     ]
    //   },
    //   {
    //     "extension": "some_ext",
    //     "extension_group": "some_group",
    //     "cmd": [
    //       {
    //         "name": "another_cmd",
    //         "dest": [
    //           {
    //             "extension": "ext_3",
    //             "extension_group": "group_3"
    //           }
    //         ]
    //       }
    //     ]
    //   }
    // ]
    pub fn check_extension_uniqueness_in_connections(&self) -> Result<()> {
        let mut errors: Vec<String> = vec![];
        let mut extensions: HashMap<String, usize> = HashMap::new();

        for (conn_idx, connection) in
            self.connections.as_ref().unwrap().iter().enumerate()
        {
            let extension = format!(
                "{}:{}:{}",
                connection.get_app_uri(),
                connection.extension_group,
                connection.extension
            );

            if let Some(idx) = extensions.get(&extension) {
                errors.push(format!(
                    "extension '{}' is defined in connection[{}] and connection[{}], merge them into one section.",
                    connection.extension, idx, conn_idx
                ));
            } else {
                extensions.insert(extension, conn_idx);
            }
        }

        if errors.is_empty() {
            Ok(())
        } else {
            Err(anyhow::anyhow!("{}", errors.join("\n")))
        }
    }
}
