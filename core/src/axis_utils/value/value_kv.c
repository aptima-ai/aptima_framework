//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_utils/value/value_kv.h"

#include <stdarg.h>
#include <stdlib.h>

#include "axis_utils/lib/alloc.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/value/value.h"

bool axis_value_kv_check_integrity(axis_value_kv_t *self) {
  axis_ASSERT(self, "Invalid argument.");

  if (axis_signature_get(&self->signature) !=
      (axis_signature_t)axis_VALUE_KV_SIGNATURE) {
    return false;
  }
  return true;
}

axis_value_kv_t *axis_value_kv_create_empty(const char *name) {
  axis_ASSERT(name, "Invalid argument.");
  return axis_value_kv_create_vempty(name);
}

axis_value_kv_t *axis_value_kv_create_vempty(const char *fmt, ...) {
  axis_ASSERT(fmt, "Invalid argument.");

  axis_value_kv_t *self = (axis_value_kv_t *)axis_malloc(sizeof(axis_value_kv_t));
  axis_ASSERT(self, "Failed to allocate memory.");

  axis_signature_set(&self->signature, (axis_signature_t)axis_VALUE_KV_SIGNATURE);

  axis_string_init(&self->key);

  va_list ap;
  va_start(ap, fmt);
  axis_string_append_from_va_list(&self->key, fmt, ap);
  va_end(ap);

  self->value = NULL;

  return self;
}

/**
 * @note Note that the ownership of @a value is moved to the value_kv.
 */
axis_value_kv_t *axis_value_kv_create(const char *name, axis_value_t *value) {
  axis_ASSERT(name && value && axis_value_check_integrity(value),
             "Invalid argument.");

  axis_value_kv_t *self = axis_value_kv_create_empty(name);
  self->value = value;

  return self;
}

axis_string_t *axis_value_kv_get_key(axis_value_kv_t *self) {
  axis_ASSERT(self && axis_value_kv_check_integrity(self), "Invalid argument.");
  return &self->key;
}

axis_value_t *axis_value_kv_get_value(axis_value_kv_t *self) {
  axis_ASSERT(self && axis_value_kv_check_integrity(self), "Invalid argument.");
  return self->value;
}

/**
 * @note Note that the ownership of @a value is moved to the value_kv.
 */
void axis_value_kv_reset_to_value(axis_value_kv_t *self, axis_value_t *value) {
  axis_ASSERT(self && axis_value_kv_check_integrity(self), "Invalid argument.");

  if (!self) {
    return;
  }

  if (self->value) {
    axis_value_destroy(self->value);
  }
  self->value = value;
}

void axis_value_kv_destroy(axis_value_kv_t *self) {
  axis_ASSERT(self && axis_value_kv_check_integrity(self), "Invalid argument.");

  axis_string_deinit(&self->key);
  if (self->value) {
    axis_value_destroy(self->value);
  }
  axis_free(self);
}

void axis_value_kv_destroy_key_only(axis_value_kv_t *self) {
  axis_ASSERT(self && axis_value_kv_check_integrity(self), "Invalid argument.");

  axis_string_deinit(&self->key);
  self->value = NULL;

  axis_free(self);
}

axis_value_kv_t *axis_value_kv_clone(axis_value_kv_t *target) {
  axis_ASSERT(target && axis_value_kv_check_integrity(target),
             "Invalid argument.");

  axis_value_kv_t *self =
      axis_value_kv_create_empty(axis_string_get_raw_str(&target->key));

  if (target->value) {
    self->value = axis_value_clone(target->value);
  }

  return self;
}

axis_string_t *axis_value_kv_to_string(axis_value_kv_t *self, axis_error_t *err) {
  axis_ASSERT(self && axis_value_kv_check_integrity(self), "Invalid argument.");

  axis_string_t *result =
      axis_string_create_formatted("%s:", axis_string_get_raw_str(&self->key));
  axis_ASSERT(result, "Invalid argument.");

  axis_string_t value_str;
  axis_string_init(&value_str);

  if (!axis_value_to_string(self->value, &value_str, err)) {
    axis_string_destroy(result);
    axis_string_deinit(&value_str);
    return NULL;
  }

  axis_string_append_formatted(result, "%s", axis_string_get_raw_str(&value_str));

  axis_string_deinit(&value_str);

  return result;
}

axis_value_kv_t *axis_value_kv_from_json(const char *key, axis_json_t *json) {
  axis_ASSERT(key, "Invalid argument.");
  axis_ASSERT(json, "Invalid argument.");

  axis_value_kv_t *kv = axis_value_kv_create_empty(key);
  axis_ASSERT(kv && axis_value_kv_check_integrity(kv), "Invalid argument.");

  kv->value = axis_value_from_json(json);
  axis_ASSERT(kv->value && axis_value_check_integrity(kv->value),
             "Invalid argument.");

  return kv;
}

void axis_value_kv_to_json(axis_json_t *json, axis_value_kv_t *kv) {
  axis_ASSERT(json, "Invalid argument.");
  axis_ASSERT(kv, "Invalid argument.");

  axis_json_object_set_new(json, axis_string_get_raw_str(&kv->key),
                          axis_value_to_json(kv->value));
}
