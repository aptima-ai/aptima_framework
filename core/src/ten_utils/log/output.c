//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "ten_utils/ten_config.h"

#include "ten_utils/lib/string.h"

#if defined(_WIN32) || defined(_WIN64)
#include <io.h>
#include <windows.h>
#else
#include <fcntl.h>
#include <unistd.h>
#endif

#include "include_internal/ten_utils/log/formatter.h"
#include "include_internal/ten_utils/log/log.h"
#include "include_internal/ten_utils/log/output.h"
#include "ten_utils/lib/file.h"

static void ten_log_output_set(ten_log_t *self,
                               const ten_log_output_func_t output_cb,
                               const ten_log_close_func_t close_cb,
                               void *user_data) {
  assert(self && "Invalid argument.");

  self->output.user_data = user_data;
  self->output.output_cb = output_cb;
  self->output.close_cb = close_cb;
}

static void ten_log_close_file_cb(void *user_data) {
  int *fd = user_data;
  assert(fd && *fd && "Invalid argument.");

#if defined(_WIN32) || defined(_WIN64)
  user_data = NULL;
  HANDLE handle = (HANDLE)_get_osfhandle(*fd);
  CloseHandle(handle);
#else
  close(*fd);
#endif

  free(fd);
}

static int *get_log_fd(const char *log_path) {
  int *fd_ptr = malloc(sizeof(int));
  assert(fd_ptr && "Failed to allocate memory.");

  FILE *fp = fopen(log_path, "ab");
  *fd_ptr = ten_file_get_fd(fp);

  return fd_ptr;
}

static void ten_log_output_to_file_cb(ten_string_t *msg, void *user_data) {
  assert(msg && "Invalid argument.");

  if (!user_data) {
    return;
  }

  ten_string_append_formatted(msg, "%s", TEN_LOG_EOL);

#if defined(_WIN32) || defined(_WIN64)
  HANDLE handle = *(HANDLE *)user_data;

  // WriteFile() is atomic for local files opened with
  // FILE_APPEND_DATA and without FILE_WRITE_DATA
  DWORD written;
  WriteFile(handle, ten_string_get_raw_str(msg), (DWORD)ten_string_len(msg),
            &written, 0);
#else
  int fd = *(int *)user_data;

  // TODO(Wei): write() is atomic for buffers less than or equal to PIPE_BUF,
  // therefore we need to have some locking mechanism here to prevent log
  // interleaved.
  write(fd, ten_string_get_raw_str(msg), ten_string_len(msg));
#endif
}

void ten_log_set_output_to_file(ten_log_t *self, const char *log_path) {
  assert(log_path && "Invalid argument.");

  int *fd = get_log_fd(log_path);
  ten_log_output_set(self, ten_log_output_to_file_cb, ten_log_close_file_cb,
                     fd);

  ten_log_set_formatter(self, ten_log_default_formatter, NULL);
}

void ten_log_output_to_stderr_cb(ten_string_t *msg, void *user_data) {
  assert(msg && "Invalid argument.");

  (void)user_data;

  ten_string_append_formatted(msg, "%s", TEN_LOG_EOL);

#if defined(_WIN32) || defined(_WIN64)
  // WriteFile() is atomic for local files opened with FILE_APPEND_DATA and
  // without FILE_WRITE_DATA
  DWORD written;
  WriteFile(GetStdHandle(STD_ERROR_HANDLE), ten_string_get_raw_str(msg),
            (DWORD)ten_string_len(msg), &written, 0);
#else
  // TODO(Wei): write() is atomic for buffers less than or equal to PIPE_BUF,
  // therefore we need to have some locking mechanism here to prevent log
  // interleaved.
  write(STDERR_FILENO, ten_string_get_raw_str(msg), ten_string_len(msg));
#endif
}

void ten_log_set_output_to_stderr(ten_log_t *self) {
  ten_log_output_set(self, ten_log_output_to_stderr_cb, NULL, NULL);

#if defined(OS_LINUX) || defined(OS_MACOS)
  ten_log_set_formatter(self, ten_log_colored_formatter, NULL);
#else
  ten_log_set_formatter(self, ten_log_default_formatter, NULL);
#endif
}
