//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_runtime/axis_config.h"

#include "include_internal/axis_runtime/addon/addon.h"
#include "axis_utils/lib/string.h"

axis_RUNTIME_PRIVATE_API bool axis_manifest_get_type_and_name(
    const char *filename, axis_ADDON_TYPE *type, axis_string_t *name,
    axis_error_t *err);
