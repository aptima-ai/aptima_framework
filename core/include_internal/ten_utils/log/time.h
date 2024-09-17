//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "ten_utils/ten_config.h"

#include <time.h>

#include "ten_utils/lib/string.h"

TEN_UTILS_PRIVATE_API void ten_log_get_time(struct tm *time_info, size_t *msec);

TEN_UTILS_PRIVATE_API void ten_log_add_time_string(ten_string_t *buf,
                                                   struct tm *time_info,
                                                   size_t msec);
