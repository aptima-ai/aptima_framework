//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include "ten_utils/ten_config.h"

#if defined(_WIN32) || defined(_WIN64)
  #include <windows.h>
#else
  #include <fcntl.h>
#endif

#include "ten_utils/log/log.h"
#include "ten_utils/macro/check.h"

void ten_log_close(void) {
  if (TEN_LOG_GLOBAL_OUTPUT->close_cb) {
    TEN_LOG_GLOBAL_OUTPUT->close_cb(TEN_LOG_GLOBAL_OUTPUT->arg);
  }

  // After closing the whatever log stream, reset it to the stderr one so that
  // the possible more logs could be dumped.
  ten_log_set_output_to_stderr();
}

void ten_log_close_aux(ten_log_t *self) {
  TEN_ASSERT(self && ten_log_check_integrity(self), "Invalid argument.");

  if (self->output && self->output->close_cb) {
    self->output->close_cb(self->output->arg);
  }

  // @{
  // After closing the whatever log stream, reset it to the stderr one so that
  // the possible more logs could be dumped.
  ten_log_output_destroy(self->output);
  ten_log_set_output_to_stderr_aux(self);
  // @}
}
