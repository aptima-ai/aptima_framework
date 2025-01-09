//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "aptima_runtime/aptima_config.h"

#include <stdbool.h>

#define aptima_REGISTER_ADDON_AS_EXTENSION(NAME, ADDON) \
  aptima_ADDON_REGISTER(extension, NAME, ADDON)

typedef struct aptima_addon_t aptima_addon_t;
typedef struct aptima_addon_host_t aptima_addon_host_t;

aptima_RUNTIME_API aptima_addon_host_t *aptima_addon_register_extension(
    const char *name, const char *base_dir, aptima_addon_t *addon,
    void *register_ctx);
