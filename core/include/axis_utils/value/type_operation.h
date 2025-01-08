//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_utils/axis_config.h"

#include "axis_utils/container/list.h"
#include "axis_utils/lib/json.h"
#include "axis_utils/value/type.h"

axis_UTILS_API axis_TYPE axis_type_from_string(const char *type_str);

axis_UTILS_API const char *axis_type_to_string(axis_TYPE type);

axis_UTILS_PRIVATE_API axis_list_t axis_type_from_json(axis_json_t *json);

axis_UTILS_PRIVATE_API bool axis_type_is_compatible(axis_TYPE actual,
                                                  axis_TYPE expected);
