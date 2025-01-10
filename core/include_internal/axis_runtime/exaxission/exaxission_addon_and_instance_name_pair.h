//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include "axis_utils/lib/json.h"
#include "axis_utils/lib/string.h"

typedef struct axis_extension_addon_and_instance_name_pair_t {
  axis_string_t addon_name;
  axis_string_t instance_name;
} axis_extension_addon_and_instance_name_pair_t;

axis_RUNTIME_PRIVATE_API axis_extension_addon_and_instance_name_pair_t *
axis_extension_addon_and_instance_name_pair_create(
    const char *extension_addon_name, const char *extension_instance_name);

axis_RUNTIME_PRIVATE_API void axis_extension_addon_and_instance_name_pair_destroy(
    axis_extension_addon_and_instance_name_pair_t *self);

axis_RUNTIME_PRIVATE_API void axis_extension_addon_and_instance_name_pair_to_json(
    axis_json_t *json, const char *key, axis_string_t *addon_name,
    axis_string_t *instance_name);
