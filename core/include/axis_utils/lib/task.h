//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "aptima_utils/aptima_config.h"

#include <stdint.h>

typedef int64_t aptima_pid_t;

/**
 * @brief Get process id.
 */
aptima_UTILS_API aptima_pid_t aptima_task_get_id();
