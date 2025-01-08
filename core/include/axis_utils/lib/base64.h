//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_utils/axis_config.h"

#include "axis_utils/lib/buf.h"
#include "axis_utils/lib/string.h"

axis_UTILS_API bool axis_base64_to_string(axis_string_t *result, axis_buf_t *buf);

axis_UTILS_API bool axis_base64_from_string(axis_string_t *str, axis_buf_t *result);
