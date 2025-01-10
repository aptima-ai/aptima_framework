//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_utils/axis_config.h"

#include "axis_utils/lib/string.h"

axis_UTILS_PRIVATE_API void axis_string_init_from_va_list(axis_string_t *self,
                                                        const char *fmt,
                                                        va_list ap);
