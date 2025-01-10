//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "ten_utils/ten_config.h"

#include <stddef.h>

TEN_UTILS_API char *ten_sanitizer_memory_strndup(const char *str, size_t size,
                                                 const char *file_name,
                                                 uint32_t lineno,
                                                 const char *func_name);
