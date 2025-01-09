//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "aptima_utils/aptima_config.h"

#include <stdbool.h>

#include "aptima_utils/lib/json.h"

typedef struct aptima_value_t aptima_value_t;

aptima_UTILS_API bool aptima_value_object_merge_with_move(aptima_value_t *dest,
                                                    aptima_value_t *src);

aptima_UTILS_API bool aptima_value_object_merge_with_clone(aptima_value_t *dest,
                                                     aptima_value_t *src);

aptima_UTILS_API bool aptima_value_object_merge_with_json(aptima_value_t *dest,
                                                    aptima_json_t *src);
