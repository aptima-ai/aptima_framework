//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "aptima_utils/aptima_config.h"

#include <stddef.h>

aptima_UTILS_API size_t aptima_terminal_get_width_in_char(void);

aptima_UTILS_API int aptima_terminal_is_terminal(int fd);
