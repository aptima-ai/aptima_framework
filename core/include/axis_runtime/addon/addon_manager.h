//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "aptima_runtime/aptima_config.h"

#include <stdbool.h>

typedef struct aptima_addon_manager_t aptima_addon_manager_t;

typedef void (*aptima_addon_registration_func_t)(void *register_ctx);

aptima_RUNTIME_API aptima_addon_manager_t *aptima_addon_manager_get_instance(void);

aptima_RUNTIME_API bool aptima_addon_manager_add_addon(
    aptima_addon_manager_t *self, const char *addon_type_str,
    const char *addon_name, aptima_addon_registration_func_t func);
