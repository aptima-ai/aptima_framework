//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include "axis_utils/lib/string.h"

typedef struct axis_app_t axis_app_t;

axis_RUNTIME_PRIVATE_API const char *axis_app_get_base_dir(axis_app_t *self);

axis_RUNTIME_PRIVATE_API axis_string_t *axis_find_app_base_dir(void);

axis_RUNTIME_PRIVATE_API void axis_app_find_and_set_base_dir(axis_app_t *self);
