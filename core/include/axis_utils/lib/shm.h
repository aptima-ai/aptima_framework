//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "aptima_utils/aptima_config.h"

#include <stddef.h>

aptima_UTILS_API void *aptima_shm_map(const char *name, size_t size);

aptima_UTILS_API size_t aptima_shm_get_size(void *addr);

aptima_UTILS_API void aptima_shm_unmap(void *addr);

aptima_UTILS_API void aptima_shm_unlink(const char *name);
