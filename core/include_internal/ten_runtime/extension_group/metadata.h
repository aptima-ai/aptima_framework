//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#pragma once

#include "ten_runtime/ten_config.h"

#include <stdbool.h>

typedef struct ten_extension_group_t ten_extension_group_t;

TEN_RUNTIME_PRIVATE_API void ten_extension_group_load_metadata(
    ten_extension_group_t *self);

TEN_RUNTIME_PRIVATE_API void ten_extension_group_merge_properties_from_graph(
    ten_extension_group_t *self);
