//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_utils/axis_config.h"

typedef struct axis_migrate_t axis_migrate_t;

axis_UTILS_PRIVATE_API void axis_migrate_task_create_and_insert(
    axis_migrate_t *migrate);
