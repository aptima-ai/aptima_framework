//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_utils/axis_config.h"

typedef void *axis_atomic_ptr_t;

axis_UTILS_API void *axis_atomic_ptr_load(volatile axis_atomic_ptr_t *a);

axis_UTILS_API void axis_atomic_ptr_store(volatile axis_atomic_ptr_t *a, void *v);
