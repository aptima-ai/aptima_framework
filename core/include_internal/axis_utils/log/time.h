//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_utils/axis_config.h"

#include <time.h>

#include "axis_utils/lib/string.h"

axis_UTILS_PRIVATE_API void axis_log_get_time(struct tm *time_info, size_t *msec);

axis_UTILS_PRIVATE_API void axis_log_add_time_string(axis_string_t *buf,
                                                   struct tm *time_info,
                                                   size_t msec);
