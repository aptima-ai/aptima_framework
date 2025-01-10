//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/ten_runtime/common/log.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "include_internal/ten_utils/lib/string.h"
#include "include_internal/ten_utils/log/formatter.h"
#include "include_internal/ten_utils/log/log.h"
#include "include_internal/ten_utils/log/output.h"
#include "ten_utils/lib/signature.h"
#include "ten_utils/lib/string.h"

bool ten_log_check_integrity(ten_log_t *self) {
  assert(self && "Invalid argument.");

  if (ten_signature_get(&self->signature) !=
      (ten_signature_t)TEN_LOG_SIGNATURE) {
    return false;
  }

  if (self->output_level == TEN_LOG_LEVEL_INVALID) {
    return false;
  }

  return true;
}

void ten_log_init(ten_log_t *self) {
  assert(self && "Invalid argument.");

  ten_signature_set(&self->signature, TEN_LOG_SIGNATURE);
  self->output_level = TEN_LOG_LEVEL_INVALID;

  ten_log_set_output_to_stderr(self);
}

ten_log_t *ten_log_create(void) {
  ten_log_t *log = malloc(sizeof(ten_log_t));
  assert(log && "Failed to allocate memory.");

  ten_log_init(log);

  return log;
}

void ten_log_deinit(ten_log_t *self) {
  assert(self && ten_log_check_integrity(self) && "Invalid argument.");

  if (self->output.close_cb) {
    self->output.close_cb(self->output.user_data);
  }
}

void ten_log_destroy(ten_log_t *self) {
  assert(self && ten_log_check_integrity(self) && "Invalid argument.");

  ten_log_deinit(self);
  free(self);
}

static const char *funcname(const char *func) { return func ? func : ""; }

const char *filename(const char *path, size_t path_len, size_t *filename_len) {
  assert(filename_len && "Invalid argument.");

  if (!path || path_len == 0) {
    *filename_len = 0;
    return "";
  }

  const char *filename = NULL;
  size_t pos = 0;

  // Start from the end of the path and go backwards.
  for (size_t i = path_len; i > 0; i--) {
    if (path[i - 1] == '/' || path[i - 1] == '\\') {
      filename = &path[i];
      pos = i;
      break;
    }
  }

  if (!filename) {
    filename = path;
    pos = 0;
  }

  // Calculate the length of the filename.
  *filename_len = path_len - pos;

  return filename;
}

void ten_log_log_from_va_list(ten_log_t *self, TEN_LOG_LEVEL level,
                              const char *func_name, const char *file_name,
                              size_t line_no, const char *fmt, va_list ap) {
  assert(self && ten_log_check_integrity(self) && "Invalid argument.");

  if (level < self->output_level) {
    return;
  }

  ten_string_t msg;
  ten_string_init_from_va_list(&msg, fmt, ap);

  ten_log_log(self, level, func_name, file_name, line_no,
              ten_string_get_raw_str(&msg));

  ten_string_deinit(&msg);
}

void ten_log_log_with_size_from_va_list(ten_log_t *self, TEN_LOG_LEVEL level,
                                        const char *func_name,
                                        size_t func_name_len,
                                        const char *file_name,
                                        size_t file_name_len, size_t line_no,
                                        const char *fmt, va_list ap) {
  assert(self && ten_log_check_integrity(self) && "Invalid argument.");

  if (level < self->output_level) {
    return;
  }

  ten_string_t msg;
  ten_string_init_from_va_list(&msg, fmt, ap);

  ten_log_log_with_size(self, level, func_name, func_name_len, file_name,
                        file_name_len, line_no, ten_string_get_raw_str(&msg),
                        ten_string_len(&msg));

  ten_string_deinit(&msg);
}

void ten_log_log_formatted(ten_log_t *self, TEN_LOG_LEVEL level,
                           const char *func_name, const char *file_name,
                           size_t line_no, const char *fmt, ...) {
  assert(self && ten_log_check_integrity(self) && "Invalid argument.");

  if (level < self->output_level) {
    return;
  }

  va_list ap;
  va_start(ap, fmt);

  ten_log_log_from_va_list(self, level, func_name, file_name, line_no, fmt, ap);

  va_end(ap);
}

void ten_log_log(ten_log_t *self, TEN_LOG_LEVEL level, const char *func_name,
                 const char *file_name, size_t line_no, const char *msg) {
  assert(self && ten_log_check_integrity(self) && "Invalid argument.");

  if (level < self->output_level) {
    return;
  }

  ten_log_log_with_size(
      self, level, func_name, func_name ? strlen(func_name) : 0, file_name,
      file_name ? strlen(file_name) : 0, line_no, msg, msg ? strlen(msg) : 0);
}

void ten_log_log_with_size(ten_log_t *self, TEN_LOG_LEVEL level,
                           const char *func_name, size_t func_name_len,
                           const char *file_name, size_t file_name_len,
                           size_t line_no, const char *msg, size_t msg_len) {
  assert(self && ten_log_check_integrity(self) && "Invalid argument.");

  if (level < self->output_level) {
    return;
  }

  ten_string_t buf;
  ten_string_init(&buf);

  if (self->formatter.format_cb) {
    self->formatter.format_cb(&buf, level, func_name, func_name_len, file_name,
                              file_name_len, line_no, msg, msg_len);
  } else {
    // Use default formatter if none is set.
    ten_log_default_formatter(&buf, level, func_name, func_name_len, file_name,
                              file_name_len, line_no, msg, msg_len);
  }

  self->output.output_cb(&buf, self->output.user_data);

  ten_string_deinit(&buf);
}
