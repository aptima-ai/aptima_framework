//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_utils/axis_config.h"

#include <stdbool.h>

#include "axis_utils/lib/string.h"
#include "axis_utils/value/value_kv.h"

axis_UTILS_API bool axis_value_to_string(axis_value_t *self, axis_string_t *str,
                                       axis_error_t *err);

axis_UTILS_API axis_value_t *axis_value_from_type_and_string(axis_TYPE type,
                                                          const char *str,
                                                          axis_error_t *err);
