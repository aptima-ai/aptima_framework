//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "aptima_utils/aptima_config.h"

#include <stdint.h>

typedef uint64_t aptima_signature_t;

aptima_UTILS_API void aptima_signature_set(aptima_signature_t *signature,
                                     aptima_signature_t value);

aptima_UTILS_API aptima_signature_t
aptima_signature_get(const aptima_signature_t *signature);
