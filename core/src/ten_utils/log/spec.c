//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include "ten_utils/ten_config.h"

#include "include_internal/ten_utils/log/spec.h"

#include "ten_utils/log/log.h"

const ten_log_t *ten_log_get_global_spec(void) { return &global_spec; }
