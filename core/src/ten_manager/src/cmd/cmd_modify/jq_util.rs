//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
use anyhow::Result;
use jaq_core::{Ctx, RcIter};
use serde_json::Value;

pub fn jq_run(input: Value, code: &str) -> Result<Value> {
    use jaq_core::load::{Arena, File, Loader};
    let arena = Arena::default();
    let loader = Loader::new(jaq_std::defs().chain(jaq_json::defs()));
    let modules = loader.load(&arena, File { path: (), code }).unwrap();
    let filter = jaq_core::Compiler::default()
        .with_funs(jaq_std::funs().chain(jaq_json::funs()))
        .compile(modules)
        .unwrap();

    let inputs = RcIter::new(core::iter::empty());
    let mut out = filter.run((Ctx::new([], &inputs), input.into()));

    if let Some(Ok(out)) = out.next() {
        return Ok(out.into());
    }

    Err(anyhow::anyhow!("empty output"))
}

#[cfg(test)]
mod tests {
    use serde_json::json;

    use crate::cmd::cmd_modify::jq_util::jq_run;

    #[test]
    fn test_run() {
        let input_json = json!({
          "name": "default",
          "auto_start": true,
          "nodes": [
            {
              "type": "extension",
              "name": "aio_http_server_python",
              "addon": "aio_http_server_python",
              "extension_group": "test",
              "property": {
                "server_port": 8002
              }
            },
            {
              "type": "extension",
              "name": "simple_echo_cpp",
              "addon": "simple_echo_cpp",
              "extension_group": "default_extension_group"
            }
          ],
          "connections": [
            {
              "extension": "aio_http_server_python",
              "cmd": [
                {
                  "name": "test",
                  "dest": [
                    {
                      "extension": "simple_echo_cpp"
                    }
                  ]
                }
              ]
            }
          ]
        });
        let code = "(.nodes[] | select(.name == \"simple_echo_cpp\") | .property) = {server_port: 8000}";

        let expected_output = json!(
            {
                "name": "default",
                "auto_start": true,
                "nodes": [
                    {
                        "type": "extension",
                        "name": "aio_http_server_python",
                        "addon": "aio_http_server_python",
                        "extension_group": "test",
                        "property": {
                            "server_port": 8002
                        }
                    },
                    {
                        "type": "extension",
                        "name": "simple_echo_cpp",
                        "addon": "simple_echo_cpp",
                        "extension_group": "default_extension_group",
                        "property": {
                            "server_port": 8000
                        }
                    }
                ],
                "connections": [
                    {
                        "extension": "aio_http_server_python",
                        "cmd": [
                            {
                                "name": "test",
                                "dest": [
                                    {
                                        "extension": "simple_echo_cpp"
                                    }
                                ]
                            }
                        ]
                    }
                ]
            }
        );

        let result = jq_run(input_json, code).unwrap();

        assert_eq!(result, expected_output);
    }
}
