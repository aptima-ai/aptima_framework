//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include "ten_utils/log/log.h"

#include "ten_utils/lib/alloc.h"
#include "ten_utils/lib/signature.h"
#include "ten_utils/macro/check.h"
#include "ten_utils/macro/memory.h"
#include "ten_utils/ten_config.h"

bool ten_log_check_integrity(ten_log_t *self) {
  TEN_ASSERT(self, "Should not happen.");

  if (ten_signature_get(&self->signature) !=
      (ten_signature_t)TEN_LOG_SIGNATURE) {
    return false;
  }
  return true;
}

void ten_log_init(ten_log_t *log) {
  TEN_ASSERT(log, "Invalid argument.");
  ten_signature_set(&log->signature, TEN_LOG_SIGNATURE);
}

ten_log_t *ten_log_create(void) {
  ten_log_t *log = ten_malloc(sizeof(ten_log_t));
  TEN_ASSERT(log, "Failed to allocate memory.");

  ten_log_init(log);

  return log;
}

void ten_log_destroy(ten_log_t *log) {
  TEN_ASSERT(log && ten_log_check_integrity(log), "Invalid argument.");

  ten_log_format_destroy(log->format);
  ten_log_output_destroy(log->output);

  TEN_FREE(log);
}

void ten_log_format_init(ten_log_format_t *format, size_t mem_width) {
  TEN_ASSERT(format, "Invalid argument.");

  format->mem_width = mem_width;
}

ten_log_format_t *ten_log_format_create(size_t mem_width) {
  ten_log_format_t *format = ten_malloc(sizeof(ten_log_format_t));
  TEN_ASSERT(format, "Failed to allocate memory.");

  format->is_allocated = true;
  ten_log_format_init(format, mem_width);

  return format;
}

void ten_log_format_destroy(ten_log_format_t *format) {
  TEN_ASSERT(format, "Invalid argument.");

  if (format->is_allocated) {
    TEN_FREE(format);
  }
}

void ten_log_output_init(ten_log_output_t *output, uint64_t mask,
                         ten_log_output_func_t output_cb,
                         ten_log_close_func_t close_cb, void *arg) {
  TEN_ASSERT(output, "Invalid argument.");

  output->mask = mask;
  output->output_cb = output_cb;
  output->close_cb = close_cb;
  output->arg = arg;
}

ten_log_output_t *ten_log_output_create(uint64_t mask,
                                        ten_log_output_func_t output_cb,
                                        ten_log_close_func_t close_cb,
                                        void *arg) {
  ten_log_output_t *output = ten_malloc(sizeof(ten_log_output_t));
  TEN_ASSERT(output, "Failed to allocate memory.");

  output->is_allocated = true;
  ten_log_output_init(output, mask, output_cb, close_cb, arg);

  return output;
}

void ten_log_output_destroy(ten_log_output_t *output) {
  TEN_ASSERT(output, "Invalid argument.");

  if (output->is_allocated) {
    TEN_FREE(output);
  }
}
