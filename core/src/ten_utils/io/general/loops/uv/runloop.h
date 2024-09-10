//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#pragma once

#include "ten_utils/ten_config.h"

typedef struct ten_migrate_t ten_migrate_t;

TEN_UTILS_PRIVATE_API void ten_migrate_task_create_and_insert(
    ten_migrate_t *migrate);
