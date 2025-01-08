//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "include_internal/axis_utils/schema/constant_str.h"
#include "include_internal/axis_utils/schema/keywords/keyword.h"
#include "include_internal/axis_utils/schema/keywords/keyword_items.h"
#include "include_internal/axis_utils/schema/keywords/keyword_properties.h"
#include "include_internal/axis_utils/schema/keywords/keyword_required.h"
#include "include_internal/axis_utils/schema/keywords/keyword_type.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/macro/check.h"

#ifdef __cplusplus
#error \
    "This file contains C99 array designated initializer, and Visual Studio C++ compiler can only support up to C89 by default, so we enable this checking to prevent any wrong inclusion of this file."
#endif

typedef axis_schema_keyword_t *(*axis_schema_keyword_create_from_value_func_t)(
    axis_schema_t *self, axis_value_t *value);

typedef struct axis_schema_keyword_info_t {
  const char *name;
  axis_schema_keyword_create_from_value_func_t from_value;
} axis_schema_keyword_info_t;

static const axis_schema_keyword_info_t axis_schema_keywords_info[] = {
    [axis_SCHEMA_KEYWORD_TYPE] =
        {
            .name = axis_SCHEMA_KEYWORD_STR_TYPE,
            .from_value = axis_schema_keyword_type_create_from_value,
        },
    [axis_SCHEMA_KEYWORD_PROPERTIES] =
        {
            .name = axis_SCHEMA_KEYWORD_STR_PROPERTIES,
            .from_value = axis_schema_keyword_properties_create_from_value,
        },
    [axis_SCHEMA_KEYWORD_ITEMS] =
        {
            .name = axis_SCHEMA_KEYWORD_STR_ITEMS,
            .from_value = axis_schema_keyword_items_create_from_value,
        },
    [axis_SCHEMA_KEYWORD_REQUIRED] =
        {
            .name = axis_SCHEMA_KEYWORD_STR_REQUIRED,
            .from_value = axis_schema_keyword_required_create_from_value,
        },
};

static const size_t axis_schema_keywords_info_size =
    sizeof(axis_schema_keywords_info) / sizeof(axis_schema_keywords_info[0]);

static inline const axis_schema_keyword_info_t *
axis_schema_keyword_info_get_by_name(const char *name) {
  for (size_t i = 0; i < axis_schema_keywords_info_size; i++) {
    if (axis_schema_keywords_info[i].name &&
        axis_c_string_is_equal(axis_schema_keywords_info[i].name, name)) {
      return &axis_schema_keywords_info[i];
    }
  }

  return NULL;
}

static inline const char *axis_schema_keyword_to_string(
    axis_SCHEMA_KEYWORD keyword) {
  if (keyword == axis_SCHEMA_KEYWORD_INVALID ||
      keyword == axis_SCHEMA_KEYWORD_LAST) {
    axis_ASSERT(0, "Invalid argument.");
    return NULL;
  }

  return axis_schema_keywords_info[keyword].name;
}
