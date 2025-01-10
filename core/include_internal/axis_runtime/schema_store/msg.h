//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include "include_internal/axis_utils/schema/schema.h"
#include "axis_utils/container/hash_handle.h"
#include "axis_utils/container/hash_table.h"
#include "axis_utils/lib/error.h"
#include "axis_utils/lib/signature.h"
#include "axis_utils/value/value.h"

#define axis_MSG_SCHEMA_SIGNATURE 0x5E2D4490ADD96568U

typedef struct axis_msg_schema_t {
  axis_signature_t signature;

  axis_string_t msg_name;
  axis_hashhandle_t hh_in_map;

  // The schema definitions are as follows:
  //
  // "api": {
  //   "data_in": [
  //     {                <== This section will be passed to
  //                          `axis_msg_schema_create`.
  //       "name": "foo",
  //       "property": {  <== This section will be stored in `property`.
  //         "foo": {
  //           "type": "string"
  //         },
  //         "bar": {
  //           "type": "int8"
  //         }
  //       }
  //     }
  //   ]
  // }
  //
  // The actual type is axis_schema_object_t, refer to
  // 'axis_schemas_parse_schema_object_for_property'.
  axis_schema_t *property;
} axis_msg_schema_t;

axis_RUNTIME_PRIVATE_API bool axis_msg_schema_check_integrity(
    axis_msg_schema_t *self);

axis_RUNTIME_PRIVATE_API axis_msg_schema_t *axis_msg_schema_create(
    axis_value_t *msg_schema_value);

axis_RUNTIME_PRIVATE_API void axis_msg_schema_destroy(axis_msg_schema_t *self);

axis_RUNTIME_PRIVATE_API void axis_msg_schema_init(axis_msg_schema_t *self,
                                                 axis_value_t *msg_schema_value);

axis_RUNTIME_PRIVATE_API void axis_msg_schema_deinit(axis_msg_schema_t *self);

/**
 * @param msg_props The property must be an object.
 */
axis_RUNTIME_API bool axis_msg_schema_adjust_properties(axis_msg_schema_t *self,
                                                      axis_value_t *msg_props,
                                                      axis_error_t *err);

/**
 * @param msg_props The property must be an object.
 */
axis_RUNTIME_API bool axis_msg_schema_validate_properties(axis_msg_schema_t *self,
                                                        axis_value_t *msg_props,
                                                        axis_error_t *err);

axis_RUNTIME_PRIVATE_API const char *axis_msg_schema_get_msg_name(
    axis_msg_schema_t *self);
