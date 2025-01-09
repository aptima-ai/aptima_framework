//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "aptima_utils/aptima_config.h"

#include <stdbool.h>
#include <stdint.h>

#include "aptima_utils/lib/error.h"
#include "aptima_utils/value/value_kv.h"

aptima_UTILS_API aptima_value_t *aptima_value_object_peek(aptima_value_t *self,
                                                 const char *key);

aptima_UTILS_API bool aptima_value_object_get_bool(aptima_value_t *self, const char *key,
                                             aptima_error_t *err);

aptima_UTILS_API const char *aptima_value_object_peek_string(aptima_value_t *self,
                                                       const char *key);

aptima_UTILS_API aptima_list_t *aptima_value_object_peek_array(aptima_value_t *self,
                                                      const char *key);

/**
 * @note Note that the ownership of @a value is moved to the value @a self.
 */
aptima_UTILS_API bool aptima_value_object_move(aptima_value_t *self, const char *key,
                                         aptima_value_t *value);
