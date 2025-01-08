//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "gtest/gtest.h"
#include "include_internal/axis_runtime/schema_store/store.h"
#include "axis_utils/lib/error.h"
#include "axis_utils/lib/json.h"
#include "axis_utils/value/value.h"
#include "axis_utils/value/value_json.h"

TEST(SchemaTest, SchemaStoreValidateProperty) {  // NOLINT
  const std::string schema_str = R"({
    "property": {
      "name": {
        "type": "string"
      },
      "age": {
        "type": "int64"
      }
    },
    "cmd_in": [
      {
        "name": "hello",
        "property": {
          "count": {
            "type": "int32"
          }
        },
        "result": {
          "_ten": {
            "detail": {
              "type": "string"
            }
          }
        }
      }
    ],
    "data_in": [
      {
        "name": "data",
        "property": {
          "fps": {
            "type": "int16"
          }
        }
      }
    ]
  })";

  auto *schema_json = axis_json_from_string(schema_str.c_str(), nullptr);
  auto *schema_value = axis_value_from_json(schema_json);
  axis_json_destroy(schema_json);

  axis_error_t err;
  axis_error_init(&err);

  axis_schema_store_t schema_store;
  axis_schema_store_init(&schema_store);
  axis_schema_store_set_schema_definition(&schema_store, schema_value, &err);
  axis_value_destroy(schema_value);

  const std::string properties_str = R"({
    "name": "demo",
    "age": 18
  })";
  auto *properties_json = axis_json_from_string(properties_str.c_str(), nullptr);
  auto *properties_value = axis_value_from_json(properties_json);
  axis_json_destroy(properties_json);

  bool success = axis_schema_store_validate_properties(&schema_store,
                                                      properties_value, &err);
  ASSERT_EQ(success, true);

  axis_error_deinit(&err);
  axis_value_destroy(properties_value);

  axis_schema_store_deinit(&schema_store);
}
