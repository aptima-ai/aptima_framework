//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/schema_store/cmd.h"

#include "include_internal/axis_runtime/common/constant_str.h"
#include "include_internal/axis_runtime/schema_store/property.h"
#include "include_internal/axis_utils/schema/schema.h"
#include "axis_utils/lib/error.h"
#include "axis_utils/lib/signature.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/log/log.h"
#include "axis_utils/macro/memory.h"
#include "axis_utils/value/value.h"
#include "axis_utils/value/value_object.h"

bool axis_cmd_schema_check_integrity(axis_cmd_schema_t *self) {
  axis_ASSERT(self, "Invalid argument.");

  if (axis_signature_get(&self->signature) != axis_CMD_SCHEMA_SIGNATURE) {
    return false;
  }

  return true;
}

axis_cmd_schema_t *axis_cmd_schema_create(axis_value_t *cmd_schema_value) {
  axis_ASSERT(cmd_schema_value && axis_value_check_integrity(cmd_schema_value),
             "Invalid argument.");

  if (!axis_value_is_object(cmd_schema_value)) {
    axis_ASSERT(0, "The schema should be an object.");
    return NULL;
  }

  axis_cmd_schema_t *self = axis_MALLOC(sizeof(axis_cmd_schema_t));
  axis_ASSERT(self, "Failed to allocate memory.");

  axis_signature_set(&self->signature, axis_CMD_SCHEMA_SIGNATURE);
  self->cmd_result_schema = NULL;

  axis_msg_schema_init(&self->hdr, cmd_schema_value);

  // "api": {
  //   "cmd_in": [
  //     {
  //       "name": "cmd_foo",
  //       "property": {
  //         "foo": {
  //           "type": "string"
  //         }
  //       },
  //       "result": {  <==
  //         "property": {
  //           "status_foo": {
  //             "type": "uint8"
  //           }
  //         }
  //       }
  //     }
  //   ]
  // }
  axis_value_t *result = axis_value_object_peek(cmd_schema_value, axis_STR_RESULT);
  if (!result) {
    axis_LOGD("No schema [result] found for cmd [%s].",
             axis_value_object_peek_string(cmd_schema_value, axis_STR_NAME));
    return self;
  }

  if (!axis_value_is_object(result)) {
    axis_ASSERT(0, "The schema [result] should be an object.");

    axis_cmd_schema_destroy(self);
    return NULL;
  }

  self->cmd_result_schema =
      axis_schemas_parse_schema_object_for_property(result);

  return self;
}

void axis_cmd_schema_destroy(axis_cmd_schema_t *self) {
  axis_ASSERT(self && axis_cmd_schema_check_integrity(self), "Invalid argument.");

  axis_signature_set(&self->signature, 0);

  axis_msg_schema_deinit(&self->hdr);
  if (self->cmd_result_schema) {
    axis_schema_destroy(self->cmd_result_schema);
    self->cmd_result_schema = NULL;
  }

  axis_FREE(self);
}

axis_string_t *axis_cmd_schema_get_cmd_name(axis_cmd_schema_t *self) {
  axis_ASSERT(self && axis_cmd_schema_check_integrity(self), "Invalid argument.");

  return &self->hdr.msg_name;
}

bool axis_cmd_schema_validate_cmd_result_properties(
    axis_cmd_schema_t *self, axis_value_t *cmd_result_props, axis_error_t *err) {
  axis_ASSERT(self && axis_cmd_schema_check_integrity(self), "Invalid argument.");
  axis_ASSERT(cmd_result_props && axis_value_check_integrity(cmd_result_props),
             "Invalid argument.");
  axis_ASSERT(err && axis_error_check_integrity(err), "Invalid argument.");

  if (!self->cmd_result_schema) {
    // No `result` schema is defined, which is permitted in APTIMA runtime.
    return true;
  }

  return axis_schema_validate_value(self->cmd_result_schema, cmd_result_props,
                                   err);
}

bool axis_cmd_schema_adjust_cmd_result_properties(axis_cmd_schema_t *self,
                                                 axis_value_t *cmd_result_props,
                                                 axis_error_t *err) {
  axis_ASSERT(self && axis_cmd_schema_check_integrity(self), "Invalid argument.");
  axis_ASSERT(cmd_result_props && axis_value_check_integrity(cmd_result_props),
             "Invalid argument.");
  axis_ASSERT(err && axis_error_check_integrity(err), "Invalid argument.");

  if (!self->cmd_result_schema) {
    // No `result` schema is defined, which is permitted in APTIMA runtime.
    return true;
  }

  return axis_schema_adjust_value_type(self->cmd_result_schema, cmd_result_props,
                                      err);
}
