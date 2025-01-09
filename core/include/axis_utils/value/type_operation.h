//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "aptima_utils/aptima_config.h"

#include "aptima_utils/container/list.h"
#include "aptima_utils/lib/json.h"
#include "aptima_utils/value/type.h"

aptima_UTILS_API aptima_TYPE aptima_type_from_string(const char *type_str);

aptima_UTILS_API const char *aptima_type_to_string(aptima_TYPE type);

aptima_UTILS_PRIVATE_API aptima_list_t aptima_type_from_json(aptima_json_t *json);

aptima_UTILS_PRIVATE_API bool aptima_type_is_compatible(aptima_TYPE actual,
                                                  aptima_TYPE expected);
