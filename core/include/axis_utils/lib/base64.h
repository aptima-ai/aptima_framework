//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "aptima_utils/aptima_config.h"

#include "aptima_utils/lib/buf.h"
#include "aptima_utils/lib/string.h"

aptima_UTILS_API bool aptima_base64_to_string(aptima_string_t *result, aptima_buf_t *buf);

aptima_UTILS_API bool aptima_base64_from_string(aptima_string_t *str, aptima_buf_t *result);
