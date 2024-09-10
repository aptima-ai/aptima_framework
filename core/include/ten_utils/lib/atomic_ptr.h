//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#pragma once

#include "ten_utils/ten_config.h"

typedef void *ten_atomic_ptr_t;

TEN_UTILS_API void *ten_atomic_ptr_load(volatile ten_atomic_ptr_t *a);

TEN_UTILS_API void ten_atomic_ptr_store(volatile ten_atomic_ptr_t *a, void *v);
