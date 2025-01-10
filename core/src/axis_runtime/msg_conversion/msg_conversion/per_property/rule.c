//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/msg_conversion/msg_conversion/per_property/rule.h"

#include "include_internal/axis_runtime/common/constant_str.h"
#include "include_internal/axis_runtime/msg/msg.h"
#include "include_internal/axis_runtime/msg_conversion/msg_conversion/per_property/fixed_value.h"
#include "axis_runtime/common/errno.h"
#include "axis_utils/lib/alloc.h"
#include "axis_utils/lib/error.h"
#include "axis_utils/lib/json.h"
#include "axis_utils/lib/smart_ptr.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/value/value.h"
#include "axis_utils/value/value_kv.h"

static void axis_msg_conversion_per_property_rule_init(
    axis_msg_conversion_per_property_rule_t *self) {
  axis_ASSERT(self, "Invalid argument.");

  axis_string_init(&self->property_path);
  self->conversion_mode =
      axis_MSG_CONVERSION_PER_PROPERTY_RULE_CONVERSION_MODE_INVALID;
}

static void axis_msg_conversion_per_property_rule_deinit(
    axis_msg_conversion_per_property_rule_t *self) {
  axis_ASSERT(self, "Invalid argument.");

  axis_string_deinit(&self->property_path);

  switch (self->conversion_mode) {
    case axis_MSG_CONVERSION_PER_PROPERTY_RULE_CONVERSION_MODE_FROM_ORIGINAL:
      axis_msg_conversion_per_property_rule_from_original_deinit(
          &self->u.from_original);
      break;
    case axis_MSG_CONVERSION_PER_PROPERTY_RULE_CONVERSION_MODE_FIXED_VALUE:
      axis_msg_conversion_per_property_rule_fixed_value_deinit(
          &self->u.fixed_value);
      break;
    default:
      break;
  }

  self->conversion_mode =
      axis_MSG_CONVERSION_PER_PROPERTY_RULE_CONVERSION_MODE_INVALID;
}

static axis_msg_conversion_per_property_rule_t *
axis_msg_conversion_per_property_rule_create(void) {
  axis_msg_conversion_per_property_rule_t *self =
      (axis_msg_conversion_per_property_rule_t *)axis_MALLOC(
          sizeof(axis_msg_conversion_per_property_rule_t));
  axis_ASSERT(self, "Failed to allocate memory.");

  axis_msg_conversion_per_property_rule_init(self);

  return self;
}

void axis_msg_conversion_per_property_rule_destroy(
    axis_msg_conversion_per_property_rule_t *self) {
  axis_ASSERT(self, "Invalid argument.");

  axis_msg_conversion_per_property_rule_deinit(self);
  axis_FREE(self);
}

bool axis_msg_conversion_per_property_rule_convert(
    axis_msg_conversion_per_property_rule_t *self, axis_shared_ptr_t *msg,
    axis_shared_ptr_t *new_msg, axis_error_t *err) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(msg && axis_msg_check_integrity(msg), "Invalid argument.");
  axis_ASSERT(new_msg && axis_msg_check_integrity(new_msg), "Invalid argument.");

  switch (self->conversion_mode) {
    case axis_MSG_CONVERSION_PER_PROPERTY_RULE_CONVERSION_MODE_FROM_ORIGINAL:
      return axis_msg_conversion_per_property_rule_from_original_convert(
          &self->u.from_original, msg, new_msg,
          axis_string_get_raw_str(&self->property_path), err);

    case axis_MSG_CONVERSION_PER_PROPERTY_RULE_CONVERSION_MODE_FIXED_VALUE:
      return axis_msg_conversion_per_property_rule_fixed_value_convert(
          &self->u.fixed_value, new_msg,
          axis_string_get_raw_str(&self->property_path), err);

    default:
      axis_ASSERT(0, "Should not happen.");
      return false;
  }
}

static axis_MSG_CONVERSION_PER_PROPERTY_RULE_CONVERSION_MODE
axis_msg_conversion_per_property_rule_conversion_mode_from_string(
    const char *conversion_mode_str, axis_error_t *err) {
  if (axis_c_string_is_equal(conversion_mode_str, axis_STR_FIXED_VALUE)) {
    return axis_MSG_CONVERSION_PER_PROPERTY_RULE_CONVERSION_MODE_FIXED_VALUE;
  } else if (axis_c_string_is_equal(conversion_mode_str,
                                   axis_STR_FROM_ORIGINAL)) {
    return axis_MSG_CONVERSION_PER_PROPERTY_RULE_CONVERSION_MODE_FROM_ORIGINAL;
  } else {
    if (err) {
      axis_error_set(err, axis_ERRNO_GENERIC, "Unsupported conversion mode '%s'",
                    conversion_mode_str);
    }
    axis_ASSERT(0, "Should not happen.");
    return axis_MSG_CONVERSION_PER_PROPERTY_RULE_CONVERSION_MODE_INVALID;
  }
}

axis_msg_conversion_per_property_rule_t *
axis_msg_conversion_per_property_rule_from_json(axis_json_t *json,
                                               axis_error_t *err) {
  axis_ASSERT(json, "Invalid argument.");

  axis_msg_conversion_per_property_rule_t *self =
      axis_msg_conversion_per_property_rule_create();

  axis_string_init_formatted(&self->property_path, "%s",
                            axis_json_object_peek_string(json, axis_STR_PATH));

  const char *conversion_mode_str =
      axis_json_object_peek_string(json, axis_STR_CONVERSION_MODE);

  self->conversion_mode =
      axis_msg_conversion_per_property_rule_conversion_mode_from_string(
          conversion_mode_str, err);

  switch (self->conversion_mode) {
    case axis_MSG_CONVERSION_PER_PROPERTY_RULE_CONVERSION_MODE_FIXED_VALUE:
      if (!axis_msg_conversion_per_property_rule_fixed_value_from_json(
              &self->u.fixed_value, json, err)) {
        axis_msg_conversion_per_property_rule_destroy(self);
        return NULL;
      }
      break;

    case axis_MSG_CONVERSION_PER_PROPERTY_RULE_CONVERSION_MODE_FROM_ORIGINAL:
      axis_msg_conversion_per_property_rule_from_original_from_json(
          &self->u.from_original, json);
      break;

    default:
      axis_ASSERT(0, "Should not happen.");
      break;
  }

  return self;
}

static const char *
axis_msg_conversion_per_property_rule_conversion_mode_to_string(
    axis_MSG_CONVERSION_PER_PROPERTY_RULE_CONVERSION_MODE conversion_mode,
    axis_error_t *err) {
  switch (conversion_mode) {
    case axis_MSG_CONVERSION_PER_PROPERTY_RULE_CONVERSION_MODE_FIXED_VALUE:
      return axis_STR_FIXED_VALUE;
    case axis_MSG_CONVERSION_PER_PROPERTY_RULE_CONVERSION_MODE_FROM_ORIGINAL:
      return axis_STR_FROM_ORIGINAL;
    default:
      if (err) {
        axis_error_set(err, axis_ERRNO_GENERIC,
                      "Unsupported conversion mode '%d'", conversion_mode);
      }
      axis_ASSERT(0, "Should not happen.");
      return NULL;
  }
}

axis_json_t *axis_msg_conversion_per_property_rule_to_json(
    axis_msg_conversion_per_property_rule_t *self, axis_error_t *err) {
  axis_ASSERT(self, "Invalid argument.");

  axis_json_t *result = axis_json_create_object();

  axis_json_object_set_new(
      result, axis_STR_CONVERSION_MODE,
      axis_json_create_string(
          axis_msg_conversion_per_property_rule_conversion_mode_to_string(
              self->conversion_mode, err)));

  axis_json_object_set_new(
      result, axis_STR_PATH,
      axis_json_create_string(axis_string_get_raw_str(&self->property_path)));

  switch (self->conversion_mode) {
    case axis_MSG_CONVERSION_PER_PROPERTY_RULE_CONVERSION_MODE_FIXED_VALUE:
      if (!axis_msg_conversion_per_property_rule_fixed_value_to_json(
              &self->u.fixed_value, result, err)) {
        axis_json_destroy(result);
        result = NULL;
      }
      break;

    case axis_MSG_CONVERSION_PER_PROPERTY_RULE_CONVERSION_MODE_FROM_ORIGINAL:
      if (!axis_msg_conversion_per_property_rule_from_original_to_json(
              &self->u.from_original, result, err)) {
        axis_json_destroy(result);
        result = NULL;
      }
      break;

    default:
      axis_ASSERT(0, "Should not happen.");
      break;
  }

  return result;
}

axis_msg_conversion_per_property_rule_t *
axis_msg_conversion_per_property_rule_from_value(axis_value_t *value,
                                                axis_error_t *err) {
  axis_ASSERT(value, "Invalid argument.");

  axis_msg_conversion_per_property_rule_t *self =
      axis_msg_conversion_per_property_rule_create();

  axis_string_set_formatted(
      &self->property_path, "%s",
      axis_value_peek_raw_str(axis_value_object_peek(value, axis_STR_PATH), err));

  const char *conversion_mode_str = axis_value_peek_raw_str(
      axis_value_object_peek(value, axis_STR_CONVERSION_MODE), err);

  self->conversion_mode =
      axis_msg_conversion_per_property_rule_conversion_mode_from_string(
          conversion_mode_str, err);

  switch (self->conversion_mode) {
    case axis_MSG_CONVERSION_PER_PROPERTY_RULE_CONVERSION_MODE_FIXED_VALUE:
      if (!axis_msg_conversion_per_property_rule_fixed_value_from_value(
              &self->u.fixed_value, value, err)) {
        axis_msg_conversion_per_property_rule_destroy(self);
        self = NULL;
      }
      break;

    case axis_MSG_CONVERSION_PER_PROPERTY_RULE_CONVERSION_MODE_FROM_ORIGINAL:
      if (!axis_msg_conversion_per_property_rule_from_original_from_value(
              &self->u.from_original, value, err)) {
        axis_msg_conversion_per_property_rule_destroy(self);
        self = NULL;
      }
      break;

    default:
      axis_ASSERT(0, "Should not happen.");
      break;
  }

  return self;
}

axis_RUNTIME_PRIVATE_API axis_value_t *
axis_msg_conversion_per_property_rule_to_value(
    axis_msg_conversion_per_property_rule_t *self, axis_error_t *err) {
  axis_ASSERT(self, "Invalid argument.");

  axis_value_t *result = axis_value_create_object_with_move(NULL);

  axis_list_push_ptr_back(
      &result->content.object,
      axis_value_kv_create(
          axis_STR_CONVERSION_MODE,
          axis_value_create_string(
              axis_msg_conversion_per_property_rule_conversion_mode_to_string(
                  self->conversion_mode, err))),
      (axis_ptr_listnode_destroy_func_t)axis_value_kv_destroy);

  axis_list_push_ptr_back(
      &result->content.object,
      axis_value_kv_create(axis_STR_PATH,
                          axis_value_create_string(
                              axis_string_get_raw_str(&self->property_path))),
      (axis_ptr_listnode_destroy_func_t)axis_value_kv_destroy);

  switch (self->conversion_mode) {
    case axis_MSG_CONVERSION_PER_PROPERTY_RULE_CONVERSION_MODE_FIXED_VALUE:
      axis_msg_conversion_per_property_rule_fixed_value_to_value(
          &self->u.fixed_value, result);
      break;

    case axis_MSG_CONVERSION_PER_PROPERTY_RULE_CONVERSION_MODE_FROM_ORIGINAL:
      axis_msg_conversion_per_property_rule_from_original_to_value(
          &self->u.from_original, result);
      break;

    default:
      axis_ASSERT(0, "Should not happen.");
      break;
  }

  return result;
}
