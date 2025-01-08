//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include <stdbool.h>

typedef struct axis_addon_manager_t axis_addon_manager_t;

typedef void (*axis_addon_registration_func_t)(void *register_ctx);

axis_RUNTIME_API axis_addon_manager_t *axis_addon_manager_get_instance(void);

axis_RUNTIME_API bool axis_addon_manager_add_addon(
    axis_addon_manager_t *self, const char *addon_type_str,
    const char *addon_name, axis_addon_registration_func_t func);
