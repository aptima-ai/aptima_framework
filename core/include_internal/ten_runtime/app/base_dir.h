//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "ten_runtime/ten_config.h"

#include "ten_utils/lib/string.h"

TEN_RUNTIME_API void ten_app_find_base_dir(ten_string_t *start_path,
                                           ten_string_t **app_path);
