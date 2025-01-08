//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_utils/axis_config.h"

#include <stdbool.h>
#include <stdint.h>

#include "axis_utils/lib/error.h"
#include "axis_utils/value/value_kv.h"

axis_UTILS_API axis_value_t *axis_value_object_peek(axis_value_t *self,
                                                 const char *key);

axis_UTILS_API bool axis_value_object_get_bool(axis_value_t *self, const char *key,
                                             axis_error_t *err);

axis_UTILS_API const char *axis_value_object_peek_string(axis_value_t *self,
                                                       const char *key);

axis_UTILS_API axis_list_t *axis_value_object_peek_array(axis_value_t *self,
                                                      const char *key);

/**
 * @note Note that the ownership of @a value is moved to the value @a self.
 */
axis_UTILS_API bool axis_value_object_move(axis_value_t *self, const char *key,
                                         axis_value_t *value);
