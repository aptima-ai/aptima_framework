//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include "include_internal/ten_runtime/msg_conversion/msg_conversion/msg_and_result_conversion_operation.h"

#include "include_internal/ten_runtime/common/constant_str.h"
#include "include_internal/ten_runtime/msg_conversion/msg_conversion_operation/base.h"
#include "ten_utils/macro/check.h"
#include "ten_utils/lib/alloc.h"
#include "ten_utils/lib/error.h"
#include "ten_utils/value/value.h"

static ten_msg_and_result_conversion_operation_t *
ten_msg_and_result_conversion_operation_create(void) {
  ten_msg_and_result_conversion_operation_t *self =
      (ten_msg_and_result_conversion_operation_t *)TEN_MALLOC(
          sizeof(ten_msg_and_result_conversion_operation_t));
  TEN_ASSERT(self, "Failed to allocate memory.");

  self->msg_operation = NULL;
  self->result_operation = NULL;

  return self;
}

void ten_msg_and_result_conversion_operation_destroy(
    ten_msg_and_result_conversion_operation_t *self) {
  TEN_ASSERT(self, "Invalid argument.");

  if (self->msg_operation) {
    ten_msg_conversion_operation_destroy(self->msg_operation);
  }
  if (self->result_operation) {
    ten_msg_conversion_operation_destroy(self->result_operation);
  }

  TEN_FREE(self);
}

ten_msg_and_result_conversion_operation_t *
ten_msg_and_result_conversion_operation_from_json(ten_json_t *json,
                                                  ten_error_t *err) {
  ten_msg_and_result_conversion_operation_t *pair =
      ten_msg_and_result_conversion_operation_create();

  pair->msg_operation = ten_msg_conversion_operation_from_json(json, err);
  if (!pair->msg_operation) {
    ten_msg_and_result_conversion_operation_destroy(pair);
    return NULL;
  }

  ten_json_t *resp_json = ten_json_object_peek(json, TEN_STR_RESULT);
  if (resp_json) {
    pair->result_operation =
        ten_msg_conversion_operation_from_json(resp_json, err);
    if (!pair->result_operation) {
      ten_msg_and_result_conversion_operation_destroy(pair);
      return NULL;
    }
  }

  return pair;
}

ten_json_t *ten_msg_and_result_conversion_operation_to_json(
    ten_msg_and_result_conversion_operation_t *self, ten_error_t *err) {
  TEN_ASSERT(self, "Invalid argument.");

  ten_json_t *msg_and_result_conversion_json =
      ten_msg_conversion_operation_to_json(self->msg_operation, err);
  if (!msg_and_result_conversion_json) {
    return NULL;
  }

  if (self->result_operation) {
    ten_json_t *result_operation_json =
        ten_msg_conversion_operation_to_json(self->result_operation, err);
    if (!result_operation_json) {
      ten_json_destroy(msg_and_result_conversion_json);
      return NULL;
    }

    ten_json_object_set_new(msg_and_result_conversion_json, TEN_STR_RESULT,
                            result_operation_json);
  }

  return msg_and_result_conversion_json;
}

ten_msg_and_result_conversion_operation_t *
ten_msg_and_result_conversion_operation_from_value(ten_value_t *value,
                                                   ten_error_t *err) {
  ten_msg_and_result_conversion_operation_t *pair =
      ten_msg_and_result_conversion_operation_create();

  pair->msg_operation = ten_msg_conversion_operation_from_value(value, err);
  if (!pair->msg_operation) {
    ten_msg_and_result_conversion_operation_destroy(pair);
    return NULL;
  }

  ten_value_t *resp_value = ten_value_object_peek(value, TEN_STR_RESULT);
  if (resp_value) {
    pair->result_operation =
        ten_msg_conversion_operation_from_value(resp_value, err);
    if (!pair->result_operation) {
      ten_msg_and_result_conversion_operation_destroy(pair);
      return NULL;
    }
  }

  return pair;
}

ten_value_t *ten_msg_and_result_conversion_operation_to_value(
    ten_msg_and_result_conversion_operation_t *self, ten_error_t *err) {
  TEN_ASSERT(self, "Invalid argument.");

  ten_value_t *msg_and_result_conversion_value =
      ten_msg_conversion_operation_to_value(self->msg_operation, err);
  if (!msg_and_result_conversion_value) {
    return NULL;
  }

  if (self->result_operation) {
    ten_value_t *result_operation_value =
        ten_msg_conversion_operation_to_value(self->result_operation, err);
    if (!result_operation_value) {
      ten_value_destroy(msg_and_result_conversion_value);
      return NULL;
    }

    ten_value_kv_t *kv =
        ten_value_kv_create(TEN_STR_RESULT, result_operation_value);

    ten_list_push_ptr_back(
        &msg_and_result_conversion_value->content.object, kv,
        (ten_ptr_listnode_destroy_func_t)ten_value_kv_destroy);
  }

  return msg_and_result_conversion_value;
}
