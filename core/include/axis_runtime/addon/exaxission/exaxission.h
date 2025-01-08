//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include <stdbool.h>

#define axis_REGISTER_ADDON_AS_EXTENSION(NAME, ADDON) \
  axis_ADDON_REGISTER(extension, NAME, ADDON)

typedef struct axis_addon_t axis_addon_t;
typedef struct axis_addon_host_t axis_addon_host_t;

axis_RUNTIME_API axis_addon_host_t *axis_addon_register_extension(
    const char *name, const char *base_dir, axis_addon_t *addon,
    void *register_ctx);
