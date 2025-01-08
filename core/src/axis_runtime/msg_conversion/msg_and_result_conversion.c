//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/msg_conversion/msg_and_result_conversion.h"

#include "include_internal/axis_runtime/common/constant_str.h"
#include "include_internal/axis_runtime/msg_conversion/msg_conversion/base.h"
#include "axis_utils/lib/alloc.h"
#include "axis_utils/lib/error.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/value/value.h"

static axis_msg_and_result_conversion_t *axis_msg_and_result_conversion_create(
    void) {
  axis_msg_and_result_conversion_t *self =
      (axis_msg_and_result_conversion_t *)axis_MALLOC(
          sizeof(axis_msg_and_result_conversion_t));
  axis_ASSERT(self, "Failed to allocate memory.");

  self->msg = NULL;
  self->result = NULL;

  return self;
}

void axis_msg_and_result_conversion_destroy(
    axis_msg_and_result_conversion_t *self) {
  axis_ASSERT(self, "Invalid argument.");

  if (self->msg) {
    axis_msg_conversion_destroy(self->msg);
  }
  if (self->result) {
    axis_msg_conversion_destroy(self->result);
  }

  axis_FREE(self);
}

axis_msg_and_result_conversion_t *axis_msg_and_result_conversion_from_json(
    axis_json_t *json, axis_error_t *err) {
  axis_msg_and_result_conversion_t *pair =
      axis_msg_and_result_conversion_create();

  pair->msg = axis_msg_conversion_from_json(json, err);
  if (!pair->msg) {
    axis_msg_and_result_conversion_destroy(pair);
    return NULL;
  }

  axis_json_t *resp_json = axis_json_object_peek(json, axis_STR_RESULT);
  if (resp_json) {
    pair->result = axis_msg_conversion_from_json(resp_json, err);
    if (!pair->result) {
      axis_msg_and_result_conversion_destroy(pair);
      return NULL;
    }
  }

  return pair;
}

axis_json_t *axis_msg_and_result_conversion_to_json(
    axis_msg_and_result_conversion_t *self, axis_error_t *err) {
  axis_ASSERT(self, "Invalid argument.");

  axis_json_t *msg_and_result_conversion_json =
      axis_msg_conversion_to_json(self->msg, err);
  if (!msg_and_result_conversion_json) {
    return NULL;
  }

  if (self->result) {
    axis_json_t *result_operation_json =
        axis_msg_conversion_to_json(self->result, err);
    if (!result_operation_json) {
      axis_json_destroy(msg_and_result_conversion_json);
      return NULL;
    }

    axis_json_object_set_new(msg_and_result_conversion_json, axis_STR_RESULT,
                            result_operation_json);
  }

  return msg_and_result_conversion_json;
}

axis_msg_and_result_conversion_t *axis_msg_and_result_conversion_from_value(
    axis_value_t *value, axis_error_t *err) {
  axis_msg_and_result_conversion_t *pair =
      axis_msg_and_result_conversion_create();

  pair->msg = axis_msg_conversion_from_value(value, err);
  if (!pair->msg) {
    axis_msg_and_result_conversion_destroy(pair);
    return NULL;
  }

  axis_value_t *resp_value = axis_value_object_peek(value, axis_STR_RESULT);
  if (resp_value) {
    pair->result = axis_msg_conversion_from_value(resp_value, err);
    if (!pair->result) {
      axis_msg_and_result_conversion_destroy(pair);
      return NULL;
    }
  }

  return pair;
}

axis_value_t *axis_msg_and_result_conversion_to_value(
    axis_msg_and_result_conversion_t *self, axis_error_t *err) {
  axis_ASSERT(self, "Invalid argument.");

  axis_value_t *msg_and_result_conversion_value =
      axis_msg_conversion_to_value(self->msg, err);
  if (!msg_and_result_conversion_value) {
    return NULL;
  }

  if (self->result) {
    axis_value_t *result_operation_value =
        axis_msg_conversion_to_value(self->result, err);
    if (!result_operation_value) {
      axis_value_destroy(msg_and_result_conversion_value);
      return NULL;
    }

    axis_value_kv_t *kv =
        axis_value_kv_create(axis_STR_RESULT, result_operation_value);

    axis_list_push_ptr_back(
        &msg_and_result_conversion_value->content.object, kv,
        (axis_ptr_listnode_destroy_func_t)axis_value_kv_destroy);
  }

  return msg_and_result_conversion_value;
}
