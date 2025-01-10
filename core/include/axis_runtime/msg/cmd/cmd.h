//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "aptima_runtime/aptima_config.h"

#include "aptima_utils/lib/error.h"
#include "aptima_utils/lib/smart_ptr.h"

aptima_RUNTIME_API aptima_shared_ptr_t *aptima_cmd_create(const char *name,
                                                 aptima_error_t *err);
