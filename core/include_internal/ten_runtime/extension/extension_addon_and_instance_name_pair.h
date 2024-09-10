//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#pragma once

#include "ten_runtime/ten_config.h"

#include "ten_utils/lib/json.h"
#include "ten_utils/lib/string.h"

typedef struct ten_extension_addon_and_instance_name_pair_t {
  ten_string_t addon_name;
  ten_string_t instance_name;
} ten_extension_addon_and_instance_name_pair_t;

TEN_RUNTIME_PRIVATE_API ten_extension_addon_and_instance_name_pair_t *
ten_extension_addon_and_instance_name_pair_create(
    const char *extension_addon_name, const char *extension_instance_name);

TEN_RUNTIME_PRIVATE_API void ten_extension_addon_and_instance_name_pair_destroy(
    ten_extension_addon_and_instance_name_pair_t *self);

TEN_RUNTIME_PRIVATE_API void ten_extension_addon_and_instance_name_pair_to_json(
    ten_json_t *json, const char *key, ten_string_t *addon_name,
    ten_string_t *instance_name);
