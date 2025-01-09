//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "aptima_runtime/aptima_config.h"

#include "aptima_utils/log/log.h"

typedef struct aptima_env_t aptima_env_t;

aptima_RUNTIME_API void aptima_env_log(aptima_env_t *self, aptima_LOG_LEVEL level,
                                 const char *func_name, const char *file_name,
                                 size_t line_no, const char *msg);
