//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_utils/axis_config.h"

#include <stdbool.h>

#include "axis_utils/lib/error.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/value/value.h"

typedef enum axis_VALUE_PATH_ITEM_TYPE {
  axis_VALUE_PATH_ITEM_TYPE_INVALID,

  axis_VALUE_PATH_ITEM_TYPE_OBJECT_ITEM,
  axis_VALUE_PATH_ITEM_TYPE_ARRAY_ITEM,
} axis_VALUE_PATH_ITEM_TYPE;

typedef struct axis_value_path_item_t {
  axis_VALUE_PATH_ITEM_TYPE type;

  union {
    axis_string_t obj_item_str;
    size_t arr_idx;
  };
} axis_value_path_item_t;

axis_UTILS_API bool axis_value_path_parse(const char *path, axis_list_t *result,
                                        axis_error_t *err);

axis_UTILS_API axis_value_t *axis_value_peek_from_path(axis_value_t *base,
                                                    const char *path,
                                                    axis_error_t *err);

axis_UTILS_API bool axis_value_set_from_path_list_with_move(axis_value_t *base,
                                                          axis_list_t *paths,
                                                          axis_value_t *value,
                                                          axis_error_t *err);

axis_UTILS_API bool axis_value_set_from_path_str_with_move(axis_value_t *base,
                                                         const char *path,
                                                         axis_value_t *value,
                                                         axis_error_t *err);
