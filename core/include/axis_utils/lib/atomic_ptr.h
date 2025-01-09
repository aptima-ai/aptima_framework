//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "aptima_utils/aptima_config.h"

typedef void *aptima_atomic_ptr_t;

aptima_UTILS_API void *aptima_atomic_ptr_load(volatile aptima_atomic_ptr_t *a);

aptima_UTILS_API void aptima_atomic_ptr_store(volatile aptima_atomic_ptr_t *a, void *v);
