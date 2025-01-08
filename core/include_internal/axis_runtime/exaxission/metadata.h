//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include <stdbool.h>

#include "axis_utils/lib/error.h"

typedef struct axis_extension_t axis_extension_t;
typedef struct axis_extension_context_t axis_extension_context_t;

axis_RUNTIME_PRIVATE_API void axis_extension_merge_properties_from_graph(
    axis_extension_t *self);

axis_RUNTIME_PRIVATE_API bool axis_extension_resolve_properties_in_graph(
    axis_extension_t *self, axis_error_t *err);

axis_RUNTIME_PRIVATE_API bool axis_extension_handle_axis_namespace_properties(
    axis_extension_t *self, axis_extension_context_t *extension_context);
