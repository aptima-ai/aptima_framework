//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

typedef struct axis_extension_t axis_extension_t;

axis_RUNTIME_PRIVATE_API const char *axis_extension_get_base_dir(
    axis_extension_t *self);
