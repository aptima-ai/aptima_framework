//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include "ten_utils/ten_config.h"

#if defined(_WIN32) || defined(_WIN64)
  #include <io.h>
  #include <windows.h>
#else
  #include <fcntl.h>
  #include <unistd.h>
#endif

#include "include_internal/ten_utils/log/platform/general/log.h"
#include "ten_utils/lib/alloc.h"
#include "ten_utils/lib/file.h"
#include "ten_utils/log/log.h"
#include "ten_utils/macro/check.h"

void ten_log_set_output_v(const uint64_t mask,
                          const ten_log_output_func_t output_cb,
                          const ten_log_close_func_t close_cb, void *arg) {
  ten_log_global_output.mask = mask;
  ten_log_global_output.arg = arg;
  ten_log_global_output.output_cb = output_cb;
  ten_log_global_output.close_cb = close_cb;
}

static void ten_log_close_file_cb(void *arg) {
  int *fd = arg;
  TEN_ASSERT(fd && *fd, "Invalid argument.");

#if defined(_WIN32) || defined(_WIN64)
  arg = NULL;
  HANDLE handle = (HANDLE)_get_osfhandle(*fd);
  CloseHandle(handle);
#else
  close(*fd);
#endif

  TEN_FREE(fd);
}

static int *get_log_fd(const char *log_path) {
  int *fd_ptr = TEN_MALLOC(sizeof(int));
  TEN_ASSERT(fd_ptr, "Failed to allocate memory.");

  FILE *fp = fopen(log_path, "ab");
  *fd_ptr = ten_file_get_fd(fp);

  return fd_ptr;
}

void ten_log_set_output_to_file(const char *log_path) {
  TEN_ASSERT(log_path, "Invalid argument.");

  int *fd = get_log_fd(log_path);

  ten_log_set_output_v(TEN_LOG_PUT_STD, ten_log_out_file_cb,
                       ten_log_close_file_cb, fd);
}

void ten_log_set_output_to_file_aux(ten_log_t *log, const char *log_path) {
  TEN_ASSERT(log && ten_log_check_integrity(log) && log_path,
             "Invalid argument.");

  int *fd = get_log_fd(log_path);

  log->format = TEN_LOG_GLOBAL_FORMAT;
  log->output = ten_log_output_create(TEN_LOG_PUT_STD, ten_log_out_file_cb,
                                      ten_log_close_file_cb, fd);
}

void ten_log_set_output_to_stderr(void) {
  ten_log_set_output_v(TEN_LOG_PUT_STD, ten_log_out_stderr_cb, NULL, NULL);
}

void ten_log_set_output_to_stderr_aux(ten_log_t *log) {
  TEN_ASSERT(log && ten_log_check_integrity(log), "Invalid argument.");

  log->format = TEN_LOG_GLOBAL_FORMAT;
  log->output = TEN_LOG_GLOBAL_OUTPUT;
}

void ten_log_save_output_spec(ten_log_output_t *output) {
  TEN_ASSERT(output, "Invalid argument.");
  if (!output) {
    return;
  }

  output->mask = TEN_LOG_GLOBAL_OUTPUT->mask;
  output->arg = TEN_LOG_GLOBAL_OUTPUT->arg;
  output->output_cb = TEN_LOG_GLOBAL_OUTPUT->output_cb;
  output->close_cb = TEN_LOG_GLOBAL_OUTPUT->close_cb;
}

void ten_log_restore_output_spec(ten_log_output_t *output) {
  TEN_ASSERT(output, "Invalid argument.");
  if (!output) {
    return;
  }

  ten_log_set_output_p(output);
}
