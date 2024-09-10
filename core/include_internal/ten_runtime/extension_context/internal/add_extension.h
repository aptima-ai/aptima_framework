//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#pragma once

#include "ten_runtime/ten_config.h"

typedef struct ten_extension_context_t ten_extension_context_t;
typedef struct ten_extension_t ten_extension_t;

TEN_RUNTIME_PRIVATE_API void ten_extension_context_add_extension(void *self_,
                                                               void *arg);
