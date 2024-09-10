//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include "include_internal/ten_runtime/msg_conversion/msg_conversion_operation/per_property/per_property.h"

#include "include_internal/ten_runtime/common/constant_str.h"
#include "include_internal/ten_runtime/msg/msg.h"
#include "include_internal/ten_runtime/msg_conversion/msg_conversion_operation/base.h"
#include "include_internal/ten_runtime/msg_conversion/msg_conversion_operation/per_property/rules.h"
#include "ten_utils/lib/alloc.h"
#include "ten_utils/lib/json.h"
#include "ten_utils/macro/check.h"
#include "ten_utils/value/value.h"
#include "ten_utils/value/value_is.h"

static ten_shared_ptr_t *ten_msg_conversion_operation_per_property_convert(
    ten_msg_conversion_operation_t *msg_conversion, ten_shared_ptr_t *msg,
    ten_error_t *err) {
  TEN_ASSERT(msg_conversion, "Should not happen.");
  TEN_ASSERT(msg && ten_msg_check_integrity(msg), "Invalid argument.");

  ten_msg_conversion_operation_per_property_t *per_property_msg_conversion =
      (ten_msg_conversion_operation_per_property_t *)msg_conversion;

  ten_shared_ptr_t *new_msg = NULL;

  if (ten_msg_get_type(msg) == TEN_MSG_TYPE_CMD_RESULT) {
    new_msg = ten_result_conversion_operation_per_property_rules_convert(
        per_property_msg_conversion->rules, msg, err);
  } else {
    new_msg = ten_msg_conversion_operation_per_property_rules_convert(
        per_property_msg_conversion->rules, msg, err);
  }

  return new_msg;
}

ten_msg_conversion_operation_per_property_t *
ten_msg_conversion_operation_per_property_create(
    ten_msg_conversion_operation_per_property_rules_t *rules) {
  ten_msg_conversion_operation_per_property_t *self =
      (ten_msg_conversion_operation_per_property_t *)TEN_MALLOC(
          sizeof(ten_msg_conversion_operation_per_property_t));
  TEN_ASSERT(self, "Failed to allocate memory.");

  self->base.type = TEN_MSG_CONVERSION_OPERATION_TYPE_PER_PROPERTY;
  self->base.operation = ten_msg_conversion_operation_per_property_convert;

  self->rules = rules;

  return self;
}

void ten_msg_conversion_operation_per_property_destroy(
    ten_msg_conversion_operation_per_property_t *self) {
  TEN_ASSERT(self, "Invalid argument.");

  ten_msg_conversion_operation_per_property_rules_destroy(self->rules);

  TEN_FREE(self);
}

ten_msg_conversion_operation_per_property_t *
ten_msg_conversion_operation_per_property_from_json(ten_json_t *json,
                                                    ten_error_t *err) {
  ten_msg_conversion_operation_per_property_rules_t *rules =
      ten_msg_conversion_operation_per_property_rules_from_json(
          ten_json_object_peek(json, TEN_STR_RULES), err);
  if (!rules) {
    return NULL;
  }

  ten_msg_conversion_operation_per_property_t *self =
      ten_msg_conversion_operation_per_property_create(rules);

  ten_json_t *keep_original_json =
      ten_json_object_peek(json, TEN_STR_KEEP_ORIGINAL);
  if (keep_original_json != NULL && ten_json_is_true(keep_original_json)) {
    self->rules->keep_original = true;
  }

  return self;
}

ten_json_t *ten_msg_conversion_operation_per_property_to_json(
    ten_msg_conversion_operation_per_property_t *self, ten_error_t *err) {
  TEN_ASSERT(self, "Invalid argument.");

  ten_json_t *json = ten_json_create_object();
  ten_json_object_set_new(json, TEN_STR_TYPE,
                          ten_json_create_string(TEN_STR_PER_PROPERTY));
  if (self->rules->keep_original) {
    ten_json_object_set_new(json, TEN_STR_KEEP_ORIGINAL,
                            ten_json_create_boolean(true));
  }

  ten_json_t *rules_json =
      ten_msg_conversion_operation_per_property_rules_to_json(self->rules, err);
  if (!rules_json) {
    ten_json_destroy(json);
    return NULL;
  }

  ten_json_object_set_new(json, TEN_STR_RULES, rules_json);

  return json;
}

ten_msg_conversion_operation_per_property_t *
ten_msg_conversion_operation_per_property_from_value(ten_value_t *value,
                                                     ten_error_t *err) {
  if (!value || !ten_value_is_object(value)) {
    return NULL;
  }

  ten_msg_conversion_operation_per_property_t *self =
      ten_msg_conversion_operation_per_property_create(
          ten_msg_conversion_operation_per_property_rules_from_value(
              ten_value_object_peek(value, TEN_STR_RULES), err));

  ten_value_t *keep_original_value =
      ten_value_object_peek(value, TEN_STR_KEEP_ORIGINAL);
  if (keep_original_value != NULL && ten_value_is_bool(keep_original_value)) {
    self->rules->keep_original = ten_value_get_bool(keep_original_value, err);
  }

  return self;
}

ten_value_t *ten_msg_conversion_operation_per_property_to_value(
    ten_msg_conversion_operation_per_property_t *self, ten_error_t *err) {
  TEN_ASSERT(self, "Invalid argument.");

  ten_value_t *result = ten_value_create_object_with_move(NULL);

  ten_value_kv_t *type_kv = ten_value_kv_create(
      TEN_STR_TYPE, ten_value_create_string(TEN_STR_PER_PROPERTY));
  ten_list_push_ptr_back(&result->content.object, type_kv,
                         (ten_ptr_listnode_destroy_func_t)ten_value_kv_destroy);

  if (self->rules->keep_original) {
    ten_value_kv_t *keep_original_kv =
        ten_value_kv_create(TEN_STR_KEEP_ORIGINAL,
                            ten_value_create_bool(self->rules->keep_original));
    ten_list_push_ptr_back(
        &result->content.object, keep_original_kv,
        (ten_ptr_listnode_destroy_func_t)ten_value_kv_destroy);
  }

  ten_value_t *rules_value =
      ten_msg_conversion_operation_per_property_rules_to_value(self->rules,
                                                               err);
  if (!rules_value) {
    ten_value_destroy(result);
    return NULL;
  }

  ten_value_kv_t *rules_kv = ten_value_kv_create(TEN_STR_RULES, rules_value);
  ten_list_push_ptr_back(&result->content.object, rules_kv,
                         (ten_ptr_listnode_destroy_func_t)ten_value_kv_destroy);

  return result;
}
