//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#pragma once

#include "ten_runtime/ten_config.h"

typedef struct ten_protocol_t ten_protocol_t;

TEN_RUNTIME_API void ten_protocol_close(ten_protocol_t *self);
