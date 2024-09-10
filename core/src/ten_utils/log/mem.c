//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include "ten_utils/ten_config.h"

#include "include_internal/ten_utils/log/dump.h"
#include "include_internal/ten_utils/log/internal.h"
#include "include_internal/ten_utils/log/spec.h"
#include "ten_utils/lib/buf.h"
#include "ten_utils/log/log.h"
#include "ten_utils/macro/check.h"

void ten_log_write_mem_d(const char *func_name, const char *file_name,
                         const size_t line, const TEN_LOG_LEVEL level,
                         const char *tag, void *buf, size_t buf_size,
                         const char *fmt, ...) {
  TEN_ASSERT(buf && buf_size && fmt, "Invalid argument.");

  const ten_log_src_location_t src_loc = {func_name, file_name, line};
  ten_buf_t mem = TEN_BUF_STATIC_INIT_WITH_DATA_UNOWNED(buf, buf_size);

  va_list va;
  va_start(va, fmt);
  ten_log_write_imp(&global_spec, &src_loc, &mem, level, tag, fmt, va);
  va_end(va);
}

void ten_log_write_mem_aux_d(const char *func_name, const char *file_name,
                             const size_t line, ten_log_t *log,
                             const TEN_LOG_LEVEL level, const char *tag,
                             void *buf, size_t buf_size, const char *fmt, ...) {
  TEN_ASSERT(buf && buf_size && fmt, "Invalid argument.");

  const ten_log_src_location_t src_loc = {func_name, file_name, line};
  ten_buf_t mem = TEN_BUF_STATIC_INIT_WITH_DATA_UNOWNED(buf, buf_size);

  va_list va;
  va_start(va, fmt);
  ten_log_write_imp(log, &src_loc, &mem, level, tag, fmt, va);
  va_end(va);
}

void ten_log_write_mem(const TEN_LOG_LEVEL level, const char *tag, void *buf,
                       size_t buf_size, const char *fmt, ...) {
  TEN_ASSERT(buf && buf_size && fmt, "Invalid argument.");

  ten_buf_t mem = TEN_BUF_STATIC_INIT_WITH_DATA_UNOWNED(buf, buf_size);

  va_list va;
  va_start(va, fmt);
  ten_log_write_imp(&global_spec, 0, &mem, level, tag, fmt, va);
  va_end(va);
}

void ten_log_write_mem_aux(ten_log_t *log, const TEN_LOG_LEVEL level,
                           const char *tag, void *buf, size_t buf_size,
                           const char *fmt, ...) {
  TEN_ASSERT(buf && buf_size && fmt, "Invalid argument.");

  ten_buf_t mem = TEN_BUF_STATIC_INIT_WITH_DATA_UNOWNED(buf, buf_size);

  va_list va;
  va_start(va, fmt);
  ten_log_write_imp(log, 0, &mem, level, tag, fmt, va);
  va_end(va);
}

void ten_log_set_mem_width(const size_t width) {
  ten_log_global_format.mem_width = width;
}
