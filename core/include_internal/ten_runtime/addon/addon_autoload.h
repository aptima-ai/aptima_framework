//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "ten_runtime/ten_config.h"

#include <stdbool.h>

#include "ten_utils/lib/error.h"

typedef struct ten_app_t ten_app_t;

TEN_RUNTIME_PRIVATE_API bool ten_addon_load_all_from_app_base_dir(
    ten_app_t *app, ten_list_t *extension_dependencies,
    ten_list_t *extension_group_dependencies, ten_list_t *protocol_dependencies,
    ten_error_t *err);

TEN_RUNTIME_PRIVATE_API bool ten_addon_load_all_from_ten_package_base_dirs(
    ten_app_t *app, ten_error_t *err);
