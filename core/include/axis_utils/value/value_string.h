//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "aptima_utils/aptima_config.h"

#include <stdbool.h>

#include "aptima_utils/lib/string.h"
#include "aptima_utils/value/value_kv.h"

aptima_UTILS_API bool aptima_value_to_string(aptima_value_t *self, aptima_string_t *str,
                                       aptima_error_t *err);

aptima_UTILS_API aptima_value_t *aptima_value_from_type_and_string(aptima_TYPE type,
                                                          const char *str,
                                                          aptima_error_t *err);
