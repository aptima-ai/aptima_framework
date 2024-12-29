//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "ten_runtime/ten_config.h"

#include <stdbool.h>

typedef struct ten_addon_manager_t ten_addon_manager_t;

typedef void (*ten_addon_registration_func_t)(void *register_ctx);

TEN_RUNTIME_API ten_addon_manager_t *ten_addon_manager_get_instance(void);

TEN_RUNTIME_API bool ten_addon_manager_add_addon(
    ten_addon_manager_t *self, const char *addon_type_str,
    const char *addon_name, ten_addon_registration_func_t func);
