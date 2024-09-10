//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include "ten_utils/ten_config.h"

#include "include_internal/ten_utils/log/platform/general/log.h"

#if defined(_WIN32) || defined(_WIN64)
  #include <windows.h>
#else
  #include <unistd.h>
#endif

#include <string.h>

#include "include_internal/ten_utils/log/eol.h"
#include "ten_utils/macro/check.h"
#include "ten_utils/macro/mark.h"

void ten_log_out_stderr_cb(const ten_log_message_t *msg, void *arg) {
  TEN_ASSERT(msg, "Invalid argument.");

  VAR_UNUSED(arg);

  const size_t eol_len = sizeof(TEN_LOG_EOL) - 1;
  memcpy(msg->buf_content_end, TEN_LOG_EOL, eol_len);

#if defined(_WIN32) || defined(_WIN64)
  // WriteFile() is atomic for local files opened with FILE_APPEND_DATA and
  // without FILE_WRITE_DATA
  DWORD written;
  WriteFile(GetStdHandle(STD_ERROR_HANDLE), msg->buf_start,
            (DWORD)(msg->buf_content_end - msg->buf_start + eol_len), &written,
            0);
#else
  // write() is atomic for buffers less than or equal to PIPE_BUF.
  RETVAL_UNUSED(
      write(STDERR_FILENO, msg->buf_start,
            (size_t)(msg->buf_content_end - msg->buf_start) + eol_len));
#endif
}

void ten_log_out_file_cb(const ten_log_message_t *msg, void *arg) {
  TEN_ASSERT(msg, "Invalid argument.");

  if (!arg) {
    return;
  }

  const size_t eol_len = sizeof(TEN_LOG_EOL) - 1;
  memcpy(msg->buf_content_end, TEN_LOG_EOL, eol_len);

#if defined(_WIN32) || defined(_WIN64)
  HANDLE handle = *(HANDLE *)arg;

  // WriteFile() is atomic for local files opened with
  // FILE_APPEND_DATA and without FILE_WRITE_DATA
  DWORD written;
  WriteFile(handle, msg->buf_start,
            (DWORD)(msg->buf_content_end - msg->buf_start + eol_len), &written,
            0);
#else
  int fd = *(int *)arg;

  // write() is atomic for buffers less than or equal to PIPE_BUF.
  RETVAL_UNUSED(
      write(fd, msg->buf_start,
            (size_t)(msg->buf_content_end - msg->buf_start) + eol_len));
#endif
}
