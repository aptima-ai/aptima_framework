//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#pragma once

#include "ten_utils/ten_config.h"

#include <stdarg.h>

#include "internal.h"
#include "ten_utils/lib/buf.h"
#include "ten_utils/log/log.h"

TEN_UTILS_PRIVATE_API void ten_log_write_imp(const ten_log_t *log,
                                             const ten_log_src_location_t *src,
                                             const ten_buf_t *mem,
                                             TEN_LOG_LEVEL level,
                                             const char *tag, const char *fmt,
                                             va_list va);
