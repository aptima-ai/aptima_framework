//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#pragma once

#include "ten_runtime/ten_config.h"

#include <stdbool.h>

#include "ten_utils/lib/error.h"

typedef struct ten_extension_t ten_extension_t;
typedef struct ten_extension_context_t ten_extension_context_t;

TEN_RUNTIME_PRIVATE_API void ten_extension_merge_properties_from_graph(
    ten_extension_t *self);

TEN_RUNTIME_PRIVATE_API bool ten_extension_resolve_properties_in_graph(
    ten_extension_t *self, ten_error_t *err);

TEN_RUNTIME_PRIVATE_API bool ten_extension_handle_ten_namespace_properties(
    ten_extension_t *self, ten_extension_context_t *extension_context);
