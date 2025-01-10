//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include "include_internal/axis_runtime/schema_store/msg.h"
#include "axis_utils/lib/signature.h"
#include "axis_utils/value/value.h"

#define axis_CMD_SCHEMA_SIGNATURE 0x740A46778CEC4CE8U

typedef struct axis_schema_t axis_schema_t;

// The schema definitions are as follows:
//
// "api": {
//   "cmd_in": [
//     {                         <== This section will be passed to
//                                   `axis_cmd_schema_create`.
//       "name": "cmd_foo",
//       "property": {           <== Stored in `hdr`.
//         "foo": {
//           "type": "string"
//         }
//       },
//       "result": {
//         "property": {         <== Stored in `cmd_result_schema`.
//           "status_foo": {
//             "type": "uint8"
//           }
//         }
//       }
//     }
//   ]
// }
typedef struct axis_cmd_schema_t {
  // This field must be the first field, as the `hdr.hh_in_map` will be used to
  // locate the address of the `axis_cmd_schema_t`.
  axis_msg_schema_t hdr;

  axis_signature_t signature;

  axis_schema_t *cmd_result_schema;
} axis_cmd_schema_t;

axis_RUNTIME_PRIVATE_API bool axis_cmd_schema_check_integrity(
    axis_cmd_schema_t *self);

axis_RUNTIME_PRIVATE_API axis_cmd_schema_t *axis_cmd_schema_create(
    axis_value_t *cmd_schema);

axis_RUNTIME_PRIVATE_API void axis_cmd_schema_destroy(axis_cmd_schema_t *self);

axis_RUNTIME_PRIVATE_API axis_string_t *axis_cmd_schema_get_cmd_name(
    axis_cmd_schema_t *self);

axis_RUNTIME_PRIVATE_API bool axis_cmd_schema_validate_cmd_result_properties(
    axis_cmd_schema_t *self, axis_value_t *status_props, axis_error_t *err);

axis_RUNTIME_PRIVATE_API bool axis_cmd_schema_adjust_cmd_result_properties(
    axis_cmd_schema_t *self, axis_value_t *status_props, axis_error_t *err);
