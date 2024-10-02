//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "ten_runtime/ten_config.h"

#include "ten_utils/lib/string.h"

typedef struct ten_extension_t ten_extension_t;

TEN_RUNTIME_PRIVATE_API ten_string_t *ten_extension_find_base_dir(
    const char *name);

TEN_RUNTIME_PRIVATE_API ten_string_t *ten_extension_get_base_dir(
    ten_extension_t *self);
