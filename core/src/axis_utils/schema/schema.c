//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_utils/schema/schema.h"

#include "include_internal/axis_utils/schema/keywords/keyword.h"
#include "include_internal/axis_utils/schema/keywords/keyword_type.h"
#include "include_internal/axis_utils/schema/keywords/keywords_info.h"
#include "include_internal/axis_utils/schema/types/schema_array.h"
#include "include_internal/axis_utils/schema/types/schema_object.h"
#include "include_internal/axis_utils/schema/types/schema_primitive.h"
#include "axis_runtime/common/errno.h"
#include "axis_utils/container/hash_handle.h"
#include "axis_utils/container/hash_table.h"
#include "axis_utils/lib/error.h"
#include "axis_utils/lib/json.h"
#include "axis_utils/lib/signature.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/macro/field.h"
#include "axis_utils/macro/memory.h"
#include "axis_utils/value/type.h"
#include "axis_utils/value/type_operation.h"
#include "axis_utils/value/value.h"
#include "axis_utils/value/value_is.h"
#include "axis_utils/value/value_object.h"

bool axis_schema_error_check_integrity(axis_schema_error_t *self) {
  axis_ASSERT(self, "Invalid argument.");

  if (axis_signature_get(&self->signature) != axis_SCHEMA_ERROR_SIGNATURE) {
    return false;
  }

  if (!self->err) {
    return false;
  }

  return true;
}

void axis_schema_error_init(axis_schema_error_t *self, axis_error_t *err) {
  axis_ASSERT(self && err, "Invalid argument.");

  axis_signature_set(&self->signature, axis_SCHEMA_ERROR_SIGNATURE);
  self->err = err;
  axis_string_init(&self->path);
}

void axis_schema_error_deinit(axis_schema_error_t *self) {
  axis_ASSERT(self && axis_schema_error_check_integrity(self),
             "Invalid argument.");

  axis_signature_set(&self->signature, 0);
  self->err = NULL;
  axis_string_deinit(&self->path);
}

void axis_schema_error_reset(axis_schema_error_t *self) {
  axis_ASSERT(self && axis_schema_error_check_integrity(self),
             "Invalid argument.");

  axis_string_clear(&self->path);
  axis_error_reset(self->err);
}

bool axis_schema_check_integrity(axis_schema_t *self) {
  axis_ASSERT(self, "Invalid argument.");

  if (axis_signature_get(&self->signature) != axis_SCHEMA_SIGNATURE) {
    return false;
  }

  return true;
}

void axis_schema_init(axis_schema_t *self) {
  axis_ASSERT(self, "Invalid argument.");

  axis_signature_set(&self->signature, axis_SCHEMA_SIGNATURE);
  self->keyword_type = NULL;
  axis_hashtable_init(&self->keywords,
                     offsetof(axis_schema_keyword_t, hh_in_keyword_map));
}

void axis_schema_deinit(axis_schema_t *self) {
  axis_ASSERT(self && axis_schema_check_integrity(self), "Invalid argument.");

  axis_signature_set(&self->signature, 0);
  axis_hashtable_deinit(&self->keywords);
}

static axis_schema_t *axis_schema_create_by_type(const char *type) {
  axis_ASSERT(type && strlen(type) > 0, "Invalid argument.");

  axis_TYPE schema_type = axis_type_from_string(type);
  switch (schema_type) {
    case axis_TYPE_OBJECT: {
      axis_schema_object_t *self = axis_schema_object_create();
      if (!self) {
        return NULL;
      }

      return &self->hdr;
    }

    case axis_TYPE_ARRAY: {
      axis_schema_array_t *self = axis_schema_array_create();
      if (!self) {
        return NULL;
      }

      return &self->hdr;
    }

    case axis_TYPE_INT8:
    case axis_TYPE_INT16:
    case axis_TYPE_INT32:
    case axis_TYPE_INT64:
    case axis_TYPE_UINT8:
    case axis_TYPE_UINT16:
    case axis_TYPE_UINT32:
    case axis_TYPE_UINT64:
    case axis_TYPE_FLOAT32:
    case axis_TYPE_FLOAT64:
    case axis_TYPE_BOOL:
    case axis_TYPE_STRING:
    case axis_TYPE_BUF:
    case axis_TYPE_PTR: {
      axis_schema_primitive_t *self = axis_schema_primitive_create();
      if (!self) {
        return NULL;
      }

      return &self->hdr;
    }

    default:
      axis_ASSERT(0, "Invalid schema type, %s.", type);
      return NULL;
  }
}

static void axis_schema_append_keyword(axis_schema_t *self,
                                      axis_schema_keyword_t *keyword) {
  axis_ASSERT(self && keyword, "Invalid argument.");
  axis_ASSERT(axis_schema_keyword_check_integrity(keyword), "Invalid argument.");

  axis_hashtable_add_int(&self->keywords, &keyword->hh_in_keyword_map,
                        (int32_t *)&keyword->type, keyword->destroy);
}

axis_schema_t *axis_schema_create_from_json(axis_json_t *json) {
  axis_ASSERT(json && axis_json_is_object(json), "Invalid argument.");

  axis_value_t *value = axis_value_from_json(json);
  axis_ASSERT(value && axis_value_is_object(value), "Should not happen.");

  axis_schema_t *self = axis_schema_create_from_value(value);
  axis_value_destroy(value);

  return self;
}

axis_schema_t *axis_schema_create_from_value(axis_value_t *value) {
  axis_ASSERT(value && axis_value_is_object(value), "Invalid argument.");

  const char *schema_type =
      axis_value_object_peek_string(value, axis_SCHEMA_KEYWORD_STR_TYPE);
  if (schema_type == NULL) {
    axis_ASSERT(0, "The schema should have a type.");
    return NULL;
  }

  axis_schema_t *self = axis_schema_create_by_type(schema_type);
  if (!self) {
    return NULL;
  }

  axis_value_object_foreach(value, iter) {
    axis_value_kv_t *field_kv = axis_ptr_listnode_get(iter.node);
    axis_ASSERT(field_kv && axis_value_kv_check_integrity(field_kv),
               "Should not happen.");

    axis_string_t *field_key = &field_kv->key;
    axis_value_t *field_value = field_kv->value;

    const axis_schema_keyword_info_t *keyword_info =
        axis_schema_keyword_info_get_by_name(axis_string_get_raw_str(field_key));
    axis_ASSERT(keyword_info, "Should not happen.");

    if (keyword_info->from_value == NULL) {
      axis_ASSERT(0, "Should not happen.");
      continue;
    }

    axis_schema_keyword_t *keyword = keyword_info->from_value(self, field_value);
    axis_ASSERT(keyword && axis_schema_keyword_check_integrity(keyword),
               "Should not happen.");

    axis_schema_append_keyword(self, keyword);
  }

  return self;
}

void axis_schema_destroy(axis_schema_t *self) {
  axis_ASSERT(self && axis_schema_check_integrity(self), "Invalid argument.");

  axis_schema_keyword_type_t *keyword_type = self->keyword_type;
  axis_ASSERT(
      keyword_type && axis_schema_keyword_type_check_integrity(keyword_type),
      "Invalid argument.");

  switch (keyword_type->type) {
    case axis_TYPE_OBJECT: {
      axis_schema_object_t *schema_object = (axis_schema_object_t *)self;
      axis_ASSERT(axis_schema_object_check_integrity(schema_object),
                 "Invalid argument.");

      axis_schema_object_destroy(schema_object);
      break;
    }

    case axis_TYPE_ARRAY: {
      axis_schema_array_t *schema_array = (axis_schema_array_t *)self;
      axis_ASSERT(axis_schema_array_check_integrity(schema_array),
                 "Invalid argument.");

      axis_schema_array_destroy(schema_array);
      break;
    }

    default: {
      axis_schema_primitive_t *schema_primitive = (axis_schema_primitive_t *)self;
      axis_ASSERT(axis_schema_primitive_check_integrity(schema_primitive),
                 "Invalid argument.");

      axis_schema_primitive_destroy(schema_primitive);
      break;
    }
  }
}

bool axis_schema_validate_value_with_schema_error(
    axis_schema_t *self, axis_value_t *value, axis_schema_error_t *schema_err) {
  axis_ASSERT(self && axis_schema_check_integrity(self), "Invalid argument.");
  axis_ASSERT(value && axis_value_check_integrity(value), "Invalid argument.");
  axis_ASSERT(schema_err && axis_schema_error_check_integrity(schema_err),
             "Invalid argument.");

  axis_hashtable_foreach(&self->keywords, iter) {
    axis_schema_keyword_t *keyword = CONTAINER_OF_FROM_OFFSET(
        iter.node, offsetof(axis_schema_keyword_t, hh_in_keyword_map));
    axis_ASSERT(keyword && axis_schema_keyword_check_integrity(keyword),
               "Should not happen.");

    bool success = keyword->validate_value(keyword, value, schema_err);
    if (!success) {
      return false;
    }
  }

  return true;
}

bool axis_schema_validate_value(axis_schema_t *self, axis_value_t *value,
                               axis_error_t *err) {
  axis_ASSERT(self && axis_schema_check_integrity(self), "Invalid argument.");

  bool new_err = false;
  if (!err) {
    err = axis_error_create();
    new_err = true;
  } else {
    axis_ASSERT(axis_error_check_integrity(err), "Invalid argument.");
  }

  bool result = false;
  if (!value) {
    axis_error_set(err, axis_ERRNO_GENERIC, "Value is required.");
    goto done;
  }

  axis_schema_error_t err_ctx;
  axis_schema_error_init(&err_ctx, err);
  result = axis_schema_validate_value_with_schema_error(self, value, &err_ctx);
  if (!result && !axis_string_is_empty(&err_ctx.path)) {
    axis_error_prepend_errmsg(err,
                             "%s: ", axis_string_get_raw_str(&err_ctx.path));
  }

  axis_schema_error_deinit(&err_ctx);

done:
  if (new_err) {
    axis_error_destroy(err);
  }

  return result;
}

bool axis_schema_adjust_value_type_with_schema_error(
    axis_schema_t *self, axis_value_t *value, axis_schema_error_t *schema_err) {
  axis_ASSERT(self && axis_schema_check_integrity(self), "Invalid argument.");
  axis_ASSERT(value && axis_value_check_integrity(value), "Invalid argument.");
  axis_ASSERT(schema_err && axis_schema_error_check_integrity(schema_err),
             "Invalid argument.");

  axis_hashtable_foreach(&self->keywords, iter) {
    axis_schema_keyword_t *keyword = CONTAINER_OF_FROM_OFFSET(
        iter.node, offsetof(axis_schema_keyword_t, hh_in_keyword_map));
    axis_ASSERT(keyword && axis_schema_keyword_check_integrity(keyword),
               "Should not happen.");

    bool success = keyword->adjust_value(keyword, value, schema_err);
    if (!success) {
      return false;
    }
  }

  return true;
}

bool axis_schema_adjust_value_type(axis_schema_t *self, axis_value_t *value,
                                  axis_error_t *err) {
  axis_ASSERT(self && axis_schema_check_integrity(self), "Invalid argument.");

  bool new_err = false;
  if (!err) {
    err = axis_error_create();
    new_err = true;
  } else {
    axis_ASSERT(axis_error_check_integrity(err), "Invalid argument.");
  }

  bool result = false;

  if (!value) {
    axis_error_set(err, axis_ERRNO_GENERIC, "Value is required.");
    goto done;
  }

  axis_schema_error_t err_ctx;
  axis_schema_error_init(&err_ctx, err);
  result =
      axis_schema_adjust_value_type_with_schema_error(self, value, &err_ctx);

  if (!result && !axis_string_is_empty(&err_ctx.path)) {
    axis_error_prepend_errmsg(err,
                             "%s: ", axis_string_get_raw_str(&err_ctx.path));
  }

  axis_schema_error_deinit(&err_ctx);

done:
  if (new_err) {
    axis_error_destroy(err);
  }

  return result;
}

static axis_schema_keyword_t *axis_schema_peek_keyword_by_type(
    axis_schema_t *self, axis_SCHEMA_KEYWORD keyword_type) {
  axis_ASSERT(self && axis_schema_check_integrity(self), "Invalid argument.");
  axis_ASSERT(keyword_type > axis_SCHEMA_KEYWORD_INVALID &&
                 keyword_type < axis_SCHEMA_KEYWORD_LAST,
             "Invalid argument.");

  axis_hashhandle_t *hh =
      axis_hashtable_find_int(&self->keywords, (int32_t *)&keyword_type);
  if (!hh) {
    return NULL;
  }

  return CONTAINER_OF_FROM_OFFSET(hh, self->keywords.hh_offset);
}

bool axis_schema_is_compatible_with_schema_error(
    axis_schema_t *self, axis_schema_t *target, axis_schema_error_t *schema_err) {
  axis_ASSERT(self && axis_schema_check_integrity(self), "Invalid argument.");
  axis_ASSERT(target && axis_schema_check_integrity(target), "Invalid argument.");
  axis_ASSERT(schema_err && axis_schema_error_check_integrity(schema_err),
             "Invalid argument.");

  bool result = true;

  // The schema 'type' should be checked first, there is no need to check other
  // keywords if the type is incompatible.
  for (axis_SCHEMA_KEYWORD keyword_type = axis_SCHEMA_KEYWORD_TYPE;
       keyword_type < axis_SCHEMA_KEYWORD_LAST; keyword_type++) {
    axis_schema_keyword_t *source_keyword =
        axis_schema_peek_keyword_by_type(self, keyword_type);
    axis_schema_keyword_t *target_keyword =
        axis_schema_peek_keyword_by_type(target, keyword_type);

    // It's OK if some source keyword or target keyword is missing, such as
    // 'required' keyword; if the source schema has 'required' keyword but the
    // target does not, it's compatible.
    if (source_keyword) {
      result = source_keyword->is_compatible(source_keyword, target_keyword,
                                             schema_err);
    } else if (target_keyword) {
      result = target_keyword->is_compatible(NULL, target_keyword, schema_err);
    } else {
      continue;
    }

    if (!result) {
      break;
    }
  }

  return result;
}

bool axis_schema_is_compatible(axis_schema_t *self, axis_schema_t *target,
                              axis_error_t *err) {
  axis_ASSERT(self && axis_schema_check_integrity(self), "Invalid argument.");
  axis_ASSERT(target && axis_schema_check_integrity(target), "Invalid argument.");

  bool new_err = false;
  if (!err) {
    err = axis_error_create();
    new_err = true;
  } else {
    axis_ASSERT(axis_error_check_integrity(err), "Invalid argument.");
  }

  axis_schema_error_t err_ctx;
  axis_schema_error_init(&err_ctx, err);

  bool result =
      axis_schema_is_compatible_with_schema_error(self, target, &err_ctx);
  if (!result && !axis_string_is_empty(&err_ctx.path)) {
    axis_error_prepend_errmsg(err,
                             "%s: ", axis_string_get_raw_str(&err_ctx.path));
  }

  axis_schema_error_deinit(&err_ctx);

  if (new_err) {
    axis_error_destroy(err);
  }

  return result;
}

axis_schema_t *axis_schema_create_from_json_str(const char *json_string,
                                              const char **err_msg) {
  axis_ASSERT(json_string, "Invalid argument.");

  axis_schema_t *schema = NULL;

  axis_error_t err;
  axis_error_init(&err);

  axis_json_t *schema_json = axis_json_from_string(json_string, &err);
  do {
    if (!schema_json) {
      break;
    }

    if (!axis_json_is_object(schema_json)) {
      axis_error_set(&err, axis_ERRNO_GENERIC, "Invalid schema json.");
      break;
    }

    schema = axis_schema_create_from_json(schema_json);
  } while (0);

  if (schema_json) {
    axis_json_destroy(schema_json);
  }

  if (!axis_error_is_success(&err)) {
    *err_msg = axis_STRDUP(axis_error_errmsg(&err));
  }

  axis_error_deinit(&err);

  return schema;
}

bool axis_schema_adjust_and_validate_json_str(axis_schema_t *self,
                                             const char *json_string,
                                             const char **err_msg) {
  axis_ASSERT(self && axis_schema_check_integrity(self), "Invalid argument.");
  axis_ASSERT(json_string, "Invalid argument.");

  axis_error_t err;
  axis_error_init(&err);

  axis_json_t *json = axis_json_from_string(json_string, &err);
  axis_value_t *value = NULL;
  do {
    if (!json) {
      break;
    }

    value = axis_value_from_json(json);
    if (!value) {
      axis_error_set(&err, axis_ERRNO_GENERIC, "Failed to parse JSON.");
      break;
    }

    if (!axis_schema_adjust_value_type(self, value, &err)) {
      break;
    }

    if (!axis_schema_validate_value(self, value, &err)) {
      break;
    }
  } while (0);

  if (json) {
    axis_json_destroy(json);
  }

  if (value) {
    axis_value_destroy(value);
  }

  bool result = axis_error_is_success(&err);
  if (!result) {
    *err_msg = axis_STRDUP(axis_error_errmsg(&err));
  }

  axis_error_deinit(&err);

  return result;
}
