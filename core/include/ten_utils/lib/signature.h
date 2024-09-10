//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#pragma once

#include "ten_utils/ten_config.h"

#include <stdint.h>

typedef uint64_t ten_signature_t;

TEN_UTILS_API void ten_signature_set(ten_signature_t *signature,
                                     ten_signature_t value);

TEN_UTILS_API ten_signature_t
ten_signature_get(const ten_signature_t *signature);
