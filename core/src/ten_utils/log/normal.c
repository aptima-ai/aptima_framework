//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include "ten_utils/ten_config.h"

#include "include_internal/ten_utils/log/dump.h"
#include "include_internal/ten_utils/log/internal.h"
#include "include_internal/ten_utils/log/spec.h"
#include "ten_utils/log/log.h"
#include "ten_utils/macro/check.h"

void ten_log_vwrite_d(const char *func_name, const char *file_name,
                      const size_t line, const TEN_LOG_LEVEL level,
                      const char *tag, const char *fmt, va_list va) {
  TEN_ASSERT(fmt, "Invalid argument.");

  const ten_log_src_location_t src = {func_name, file_name, line};

  ten_log_write_imp(&global_spec, &src, 0, level, tag, fmt, va);
}

void ten_log_write_d(const char *func_name, const char *file_name,
                     const size_t line, const TEN_LOG_LEVEL level,
                     const char *tag, const char *fmt, ...) {
  TEN_ASSERT(fmt, "Invalid argument.");

  va_list va;
  va_start(va, fmt);
  ten_log_vwrite_d(func_name, file_name, line, level, tag, fmt, va);
  va_end(va);
}

void ten_log_write_aux_d(const char *func_name, const char *file_name,
                         const size_t line, const ten_log_t *log,
                         const TEN_LOG_LEVEL level, const char *tag,
                         const char *fmt, ...) {
  TEN_ASSERT(fmt, "Invalid argument.");

  const ten_log_src_location_t src_loc = {func_name, file_name, line};

  va_list va;
  va_start(va, fmt);
  ten_log_write_imp(log, &src_loc, 0, level, tag, fmt, va);
  va_end(va);
}

void ten_log_write(const TEN_LOG_LEVEL level, const char *tag, const char *fmt,
                   ...) {
  TEN_ASSERT(fmt, "Invalid argument.");

  va_list va;
  va_start(va, fmt);
  ten_log_write_imp(&global_spec, 0, 0, level, tag, fmt, va);
  va_end(va);
}

void ten_log_write_aux(const ten_log_t *log, const TEN_LOG_LEVEL level,
                       const char *tag, const char *fmt, ...) {
  TEN_ASSERT(fmt, "Invalid argument.");

  va_list va;
  va_start(va, fmt);
  ten_log_write_imp(log, 0, 0, level, tag, fmt, va);
  va_end(va);
}
