//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
use ten_rust::interface::ten_interface_schema_resolve_definition;

#[test]
fn test_interface_ref_local() {
    let content = r#"
            [
                {
                    "name": "ia",
                    "cmd": [
                        {
                            "name": "foo"
                        }
                    ]
                },
                {
                    "name": "ib",
                    "$ref": "file://tests/test_data/interface/foo.json"
                }
            ]
          "#;

    let result = ten_interface_schema_resolve_definition(content, ".");
    assert!(result.is_ok());

    let content_json = result.unwrap();
    assert_eq!(content_json.as_array().unwrap().len(), 2);

    let ib = content_json
        .as_array()
        .unwrap()
        .get(1)
        .unwrap()
        .as_object()
        .unwrap();
    assert!(ib.contains_key("cmd"))
}

#[test]
fn test_interface_name_duplicated() {
    let content = r#"
            [
                {
                    "name": "ia",
                    "cmd": [
                        {
                            "name": "foo"
                        }
                    ]
                },
                {
                    "name": "ia",
                    "$ref": "file://tests/test_data/interface/foo.json"
                }
            ]
          "#;

    let result = ten_interface_schema_resolve_definition(content, ".");
    assert!(result.is_err());
}
