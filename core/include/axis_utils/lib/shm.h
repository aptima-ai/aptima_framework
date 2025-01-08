//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_utils/axis_config.h"

#include <stddef.h>

axis_UTILS_API void *axis_shm_map(const char *name, size_t size);

axis_UTILS_API size_t axis_shm_get_size(void *addr);

axis_UTILS_API void axis_shm_unmap(void *addr);

axis_UTILS_API void axis_shm_unlink(const char *name);
