//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_utils/axis_config.h"

#include <stddef.h>

axis_UTILS_API char *axis_strndup(const char *str, size_t size);

axis_UTILS_API char *axis_strndup_without_backtrace(const char *str, size_t size);
