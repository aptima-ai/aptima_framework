//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_utils/axis_config.h"

#include "axis_utils/log/log.h"

axis_UTILS_PRIVATE_API char axis_log_level_char(axis_LOG_LEVEL level);

axis_UTILS_PRIVATE_API void axis_log_set_output_level(axis_log_t *self,
                                                    axis_LOG_LEVEL level);
