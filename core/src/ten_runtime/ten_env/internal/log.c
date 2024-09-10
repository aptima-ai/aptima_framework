//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include "include_internal/ten_runtime/ten_env/log.h"

#include "ten_runtime/ten_env/ten_env.h"
#include "ten_utils/log/log.h"

static void ten_vlog(ten_env_t *self, const char *func_name,
                     const char *file_name, const size_t lineno,
                     TEN_LOG_LEVEL level, const char *tag, const char *fmt,
                     va_list va) {
  TEN_ASSERT(self && ten_env_check_integrity(self, true), "Should not happen.");

  ten_log_vwrite_d(func_name, file_name, lineno, level, tag, fmt, va);
}

void ten_log(ten_env_t *self, const char *func_name, const char *file_name,
             const size_t lineno, TEN_LOG_LEVEL level, const char *tag,
             const char *fmt, ...) {
  TEN_ASSERT(self && ten_env_check_integrity(self, true), "Should not happen.");

  va_list va;
  va_start(va, fmt);
  ten_vlog(self, func_name, file_name, lineno, level, tag, fmt, va);
  va_end(va);
}
