//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include "include_internal/ten_utils/schema/schema.h"

#include "include_internal/ten_utils/schema/keywords/keyword.h"
#include "include_internal/ten_utils/schema/keywords/keyword_type.h"
#include "include_internal/ten_utils/schema/keywords/keywords_info.h"
#include "include_internal/ten_utils/schema/types/schema_array.h"
#include "include_internal/ten_utils/schema/types/schema_object.h"
#include "include_internal/ten_utils/schema/types/schema_primitive.h"
#include "ten_runtime/common/errno.h"
#include "ten_utils/container/hash_handle.h"
#include "ten_utils/container/hash_table.h"
#include "ten_utils/lib/error.h"
#include "ten_utils/lib/json.h"
#include "ten_utils/lib/signature.h"
#include "ten_utils/lib/string.h"
#include "ten_utils/macro/check.h"
#include "ten_utils/macro/field.h"
#include "ten_utils/macro/memory.h"
#include "ten_utils/value/type.h"
#include "ten_utils/value/type_operation.h"
#include "ten_utils/value/value.h"
#include "ten_utils/value/value_is.h"
#include "ten_utils/value/value_object.h"

bool ten_schema_check_integrity(ten_schema_t *self) {
  TEN_ASSERT(self, "Invalid argument.");

  if (ten_signature_get(&self->signature) != TEN_SCHEMA_SIGNATURE) {
    return false;
  }

  return true;
}

void ten_schema_init(ten_schema_t *self) {
  TEN_ASSERT(self, "Invalid argument.");

  ten_signature_set(&self->signature, TEN_SCHEMA_SIGNATURE);
  self->keyword_type = NULL;
  ten_hashtable_init(&self->keywords,
                     offsetof(ten_schema_keyword_t, hh_in_keyword_map));
}

void ten_schema_deinit(ten_schema_t *self) {
  TEN_ASSERT(self && ten_schema_check_integrity(self), "Invalid argument.");

  ten_signature_set(&self->signature, 0);
  ten_hashtable_clear(&self->keywords);
}

static ten_schema_t *ten_schema_create_by_type(const char *type) {
  TEN_ASSERT(type && strlen(type) > 0, "Invalid argument.");

  TEN_TYPE schema_type = ten_type_from_string(type);
  switch (schema_type) {
    case TEN_TYPE_OBJECT: {
      ten_schema_object_t *self = ten_schema_object_create();
      if (!self) {
        return NULL;
      }

      return &self->hdr;
    }

    case TEN_TYPE_ARRAY: {
      ten_schema_array_t *self = ten_schema_array_create();
      if (!self) {
        return NULL;
      }

      return &self->hdr;
    }

    case TEN_TYPE_INT8:
    case TEN_TYPE_INT16:
    case TEN_TYPE_INT32:
    case TEN_TYPE_INT64:
    case TEN_TYPE_UINT8:
    case TEN_TYPE_UINT16:
    case TEN_TYPE_UINT32:
    case TEN_TYPE_UINT64:
    case TEN_TYPE_FLOAT32:
    case TEN_TYPE_FLOAT64:
    case TEN_TYPE_BOOL:
    case TEN_TYPE_STRING:
    case TEN_TYPE_BUF:
    case TEN_TYPE_PTR: {
      ten_schema_primitive_t *self = ten_schema_primitive_create();
      if (!self) {
        return NULL;
      }

      return &self->hdr;
    }

    default:
      TEN_ASSERT(0, "Invalid schema type.");
      return NULL;
  }
}

static void ten_schema_append_keyword(ten_schema_t *self,
                                      ten_schema_keyword_t *keyword) {
  TEN_ASSERT(self && keyword, "Invalid argument.");
  TEN_ASSERT(ten_schema_keyword_check_integrity(keyword), "Invalid argument.");

  ten_hashtable_add_int(&self->keywords, &keyword->hh_in_keyword_map,
                        (int32_t *)&keyword->type, keyword->destroy);
}

ten_schema_t *ten_schema_create_from_json(ten_json_t *json) {
  TEN_ASSERT(json && ten_json_is_object(json), "Invalid argument.");

  ten_value_t *value = ten_value_from_json(json);
  TEN_ASSERT(value && ten_value_is_object(value), "Should not happen.");

  ten_schema_t *self = ten_schema_create_from_value(value);
  ten_value_destroy(value);

  return self;
}

ten_schema_t *ten_schema_create_from_value(ten_value_t *value) {
  TEN_ASSERT(value && ten_value_is_object(value), "Invalid argument.");

  const char *schema_type =
      ten_value_object_peek_string(value, TEN_SCHEMA_KEYWORD_STR_TYPE);
  if (schema_type == NULL) {
    TEN_ASSERT(0, "The schema should have a type.");
    return NULL;
  }

  ten_schema_t *self = ten_schema_create_by_type(schema_type);
  if (!self) {
    return NULL;
  }

  ten_value_object_foreach(value, iter) {
    ten_value_kv_t *field_kv = ten_ptr_listnode_get(iter.node);
    TEN_ASSERT(field_kv && ten_value_kv_check_integrity(field_kv),
               "Should not happen.");

    ten_string_t *field_key = &field_kv->key;
    ten_value_t *field_value = field_kv->value;

    const ten_schema_keyword_info_t *keyword_info =
        ten_schema_keyword_info_get_by_name(ten_string_get_raw_str(field_key));
    TEN_ASSERT(keyword_info, "Should not happen.");

    if (keyword_info->from_value == NULL) {
      TEN_ASSERT(0, "Should not happen.");
      continue;
    }

    ten_schema_keyword_t *keyword = keyword_info->from_value(self, field_value);
    TEN_ASSERT(keyword && ten_schema_keyword_check_integrity(keyword),
               "Should not happen.");

    ten_schema_append_keyword(self, keyword);
  }

  return self;
}

void ten_schema_destroy(ten_schema_t *self) {
  TEN_ASSERT(self && ten_schema_check_integrity(self), "Invalid argument.");

  ten_schema_keyword_type_t *keyword_type = self->keyword_type;
  TEN_ASSERT(
      keyword_type && ten_schema_keyword_type_check_integrity(keyword_type),
      "Invalid argument.");

  switch (keyword_type->type) {
    case TEN_TYPE_OBJECT: {
      ten_schema_object_t *schema_object = (ten_schema_object_t *)self;
      TEN_ASSERT(ten_schema_object_check_integrity(schema_object),
                 "Invalid argument.");

      ten_schema_object_destroy(schema_object);
      break;
    }

    case TEN_TYPE_ARRAY: {
      ten_schema_array_t *schema_array = (ten_schema_array_t *)self;
      TEN_ASSERT(ten_schema_array_check_integrity(schema_array),
                 "Invalid argument.");

      ten_schema_array_destroy(schema_array);
      break;
    }

    default: {
      ten_schema_primitive_t *schema_primitive = (ten_schema_primitive_t *)self;
      TEN_ASSERT(ten_schema_primitive_check_integrity(schema_primitive),
                 "Invalid argument.");

      ten_schema_primitive_destroy(schema_primitive);
      break;
    }
  }
}

bool ten_schema_validate_value(ten_schema_t *self, ten_value_t *value,
                               ten_error_t *err) {
  TEN_ASSERT(self && ten_schema_check_integrity(self), "Invalid argument.");

  if (!value) {
    if (err) {
      ten_error_set(err, TEN_ERRNO_GENERIC, "Value is required.");
    }
    return false;
  }

  ten_hashtable_foreach(&self->keywords, iter) {
    ten_schema_keyword_t *keyword = CONTAINER_OF_FROM_OFFSET(
        iter.node, offsetof(ten_schema_keyword_t, hh_in_keyword_map));
    TEN_ASSERT(keyword && ten_schema_keyword_check_integrity(keyword),
               "Should not happen.");

    // TODO(Liu): Add item path to error message, so that user can know which
    // item is invalid. Ex: "property.a.b[0].c".
    bool success = keyword->validate_value(keyword, value, err);
    if (!success) {
      return false;
    }
  }

  return true;
}

bool ten_schema_adjust_value_type(ten_schema_t *self, ten_value_t *value,
                                  ten_error_t *err) {
  TEN_ASSERT(self && ten_schema_check_integrity(self), "Invalid argument.");

  if (!value) {
    if (err) {
      ten_error_set(err, TEN_ERRNO_GENERIC, "Value is required.");
    }
    return false;
  }

  ten_hashtable_foreach(&self->keywords, iter) {
    ten_schema_keyword_t *keyword = CONTAINER_OF_FROM_OFFSET(
        iter.node, offsetof(ten_schema_keyword_t, hh_in_keyword_map));
    TEN_ASSERT(keyword && ten_schema_keyword_check_integrity(keyword),
               "Should not happen.");

    bool success = keyword->adjust_value(keyword, value, err);
    if (!success) {
      return false;
    }
  }

  return true;
}

static ten_schema_keyword_t *ten_schema_peek_keyword_by_type(
    ten_schema_t *self, TEN_SCHEMA_KEYWORD keyword_type) {
  TEN_ASSERT(self && ten_schema_check_integrity(self), "Invalid argument.");
  TEN_ASSERT(keyword_type > TEN_SCHEMA_KEYWORD_INVALID &&
                 keyword_type < TEN_SCHEMA_KEYWORD_LAST,
             "Invalid argument.");

  ten_hashhandle_t *hh =
      ten_hashtable_find_int(&self->keywords, (int32_t *)&keyword_type);
  if (!hh) {
    return NULL;
  }

  return CONTAINER_OF_FROM_OFFSET(hh, self->keywords.hh_offset);
}

bool ten_schema_is_compatible(ten_schema_t *self, ten_schema_t *target,
                              ten_error_t *err) {
  TEN_ASSERT(self && ten_schema_check_integrity(self), "Invalid argument.");
  TEN_ASSERT(target && ten_schema_check_integrity(target), "Invalid argument.");
  TEN_ASSERT(err && ten_error_check_integrity(err), "Invalid argument.");

  bool result = true;

  // The schema 'type' should be checked first, there is no need to check other
  // keywords if the type is incompatible.
  for (TEN_SCHEMA_KEYWORD keyword_type = TEN_SCHEMA_KEYWORD_TYPE;
       keyword_type < TEN_SCHEMA_KEYWORD_LAST; keyword_type++) {
    ten_schema_keyword_t *source_keyword =
        ten_schema_peek_keyword_by_type(self, keyword_type);
    ten_schema_keyword_t *target_keyword =
        ten_schema_peek_keyword_by_type(target, keyword_type);

    // It's OK if some source keyword or target keyword is missing, such as
    // 'required' keyword; if the source schema has 'required' keyword but the
    // target does not, it's compatible.
    if (source_keyword) {
      result =
          source_keyword->is_compatible(source_keyword, target_keyword, err);
    } else if (target_keyword) {
      result = target_keyword->is_compatible(NULL, target_keyword, err);
    } else {
      continue;
    }

    if (!result) {
      break;
    }
  }

  return result;
}

ten_schema_t *ten_schema_create_from_json_string(const char *json_string,
                                                 const char **err_msg) {
  TEN_ASSERT(json_string, "Invalid argument.");

  ten_schema_t *schema = NULL;

  ten_error_t err;
  ten_error_init(&err);

  ten_json_t *schema_json = ten_json_from_string(json_string, &err);
  do {
    if (!schema_json) {
      break;
    }

    if (!ten_json_is_object(schema_json)) {
      ten_error_set(&err, TEN_ERRNO_GENERIC, "Invalid schema json.");
      break;
    }

    schema = ten_schema_create_from_json(schema_json);
  } while (0);

  if (schema_json) {
    ten_json_destroy(schema_json);
  }

  if (!ten_error_is_success(&err)) {
    *err_msg = TEN_STRDUP(ten_error_errmsg(&err));
  }

  ten_error_deinit(&err);

  return schema;
}

bool ten_schema_adjust_and_validate_json_string(ten_schema_t *self,
                                                const char *json_string,
                                                const char **err_msg) {
  TEN_ASSERT(self && ten_schema_check_integrity(self), "Invalid argument.");
  TEN_ASSERT(json_string, "Invalid argument.");

  ten_error_t err;
  ten_error_init(&err);

  ten_json_t *json = ten_json_from_string(json_string, &err);
  ten_value_t *value = NULL;
  do {
    if (!json) {
      break;
    }

    value = ten_value_from_json(json);
    if (!value) {
      ten_error_set(&err, TEN_ERRNO_GENERIC, "Failed to parse JSON.");
      break;
    }

    if (!ten_schema_adjust_value_type(self, value, &err)) {
      break;
    }

    if (!ten_schema_validate_value(self, value, &err)) {
      break;
    }
  } while (0);

  if (json) {
    ten_json_destroy(json);
  }

  if (value) {
    ten_value_destroy(value);
  }

  bool result = ten_error_is_success(&err);
  if (!result) {
    *err_msg = TEN_STRDUP(ten_error_errmsg(&err));
  }

  ten_error_deinit(&err);

  return result;
}
