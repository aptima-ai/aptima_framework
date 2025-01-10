//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include "axis_utils/lib/string.h"

axis_RUNTIME_API axis_string_t *axis_find_base_dir(const char *start_path,
                                                const char *type,
                                                const char *name);
