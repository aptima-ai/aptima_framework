//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include <stdbool.h>

typedef struct axis_extension_group_t axis_extension_group_t;

axis_RUNTIME_PRIVATE_API void axis_extension_group_load_metadata(
    axis_extension_group_t *self);

axis_RUNTIME_PRIVATE_API void axis_extension_group_merge_properties_from_graph(
    axis_extension_group_t *self);
