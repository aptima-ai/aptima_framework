//
// Copyright © 2025 Agora
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

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#define PATH_SEPARATOR '\\'
#else
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#define PATH_SEPARATOR '/'
#endif

// Function to create directories recursively.
static bool create_directories(const char *path) {
  char *path_copy = strdup(path);
  assert(path_copy && "Failed to allocate memory.");
  if (!path_copy) {
    return false;
  }

  size_t len = strlen(path_copy);
  // Remove trailing separators.
  while (len > 0 && (path_copy[len - 1] == '/' || path_copy[len - 1] == '\\')) {
    path_copy[len - 1] = '\0';
    len--;
  }

  for (size_t i = 1; i < len; i++) {
    if (path_copy[i] == '/' || path_copy[i] == '\\') {
      char temp = path_copy[i];
      path_copy[i] = '\0';

      // Skip if path is root on Unix or drive letter on Windows.
#if defined(_WIN32) || defined(_WIN64)
      if (i == 2 && path_copy[1] == ':') {
        path_copy[i] = temp;
        continue;
      }
#endif

#if defined(_WIN32) || defined(_WIN64)
      if (!CreateDirectoryA(path_copy, NULL)) {
        DWORD err = GetLastError();
        if (err != ERROR_ALREADY_EXISTS) {
          free(path_copy);
          return false;
        }
      }
#else
      if (mkdir(path_copy, 0755) != 0) {
        if (errno != EEXIST) {
          free(path_copy);
          return false;
        }
      }
#endif
      path_copy[i] = temp;
    }
  }

#if defined(_WIN32) || defined(_WIN64)
  if (!CreateDirectoryA(path_copy, NULL)) {
    DWORD err = GetLastError();
    if (err != ERROR_ALREADY_EXISTS) {
      free(path_copy);
      return false;
    }
  }
#else
  if (mkdir(path_copy, 0755) != 0) {
    if (errno != EEXIST) {
      free(path_copy);
      return false;
    }
  }
#endif

  free(path_copy);
  return true;
}

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
  assert(log_path && "log_path cannot be NULL.");

  // Duplicate the log_path to manipulate it.
  char *path_copy = strdup(log_path);
  assert(path_copy && "Failed to allocate memory.");
  if (!path_copy) {
    return NULL;
  }

  // Find the directory part of the path.
  char *last_sep = strrchr(path_copy, PATH_SEPARATOR);
  if (last_sep) {
    *last_sep = '\0';  // Terminate the string to get the directory path.

    // Create directories recursively.
    if (create_directories(path_copy) != true) {
      // Failed to create directories
      free(path_copy);
      return NULL;
    }
  }

  free(path_copy);

  // Now, attempt to open the file.
  FILE *fp = fopen(log_path, "ab");
  if (!fp) {
    // Handle fopen failure.
    return NULL;
  }

  int *fd_ptr = malloc(sizeof(int));
  assert(fd_ptr && "Failed to allocate memory.");
  if (!fd_ptr) {
    (void)fclose(fp);
    return NULL;
  }

  *fd_ptr = ten_file_get_fd(fp);
  return fd_ptr;
}

void ten_log_output_to_file_cb(ten_string_t *msg, void *user_data) {
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
  ssize_t bytes_written =
      write(fd, ten_string_get_raw_str(msg), ten_string_len(msg));
  if (bytes_written == -1) {
    perror("write failed");
  } else if ((size_t)bytes_written < ten_string_len(msg)) {
    (void)fprintf(stderr, "Partial write: only %zd of %zu bytes written\n",
                  bytes_written, ten_string_len(msg));
  }
#endif
}

void ten_log_set_output_to_file(ten_log_t *self, const char *log_path) {
  assert(log_path && "Invalid argument.");

  int *fd = get_log_fd(log_path);
  if (!fd) {
    // Failed to open log file or create directories.
    ten_log_set_output_to_stderr(self);
    return;
  }

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
  ssize_t bytes_written =
      write(STDERR_FILENO, ten_string_get_raw_str(msg), ten_string_len(msg));
  if (bytes_written == -1) {
    perror("write failed");
  } else if ((size_t)bytes_written < ten_string_len(msg)) {
    (void)fprintf(stderr, "Partial write: only %zd of %zu bytes written\n",
                  bytes_written, ten_string_len(msg));
  }
#endif
}

void ten_log_set_output_to_stderr(ten_log_t *self) {
  ten_log_output_set(self, ten_log_output_to_stderr_cb, NULL, NULL);

#if defined(OS_LINUX) || defined(OS_MACOS)
  ten_log_set_formatter(self, ten_log_colored_formatter, NULL);
  // ten_log_set_formatter(self, ten_log_default_formatter, NULL);
#else
  ten_log_set_formatter(self, ten_log_default_formatter, NULL);
#endif
}

void ten_log_output_to_file_deinit(ten_log_t *self) {
  assert(self && "Invalid argument.");
  assert(self->output.output_cb == ten_log_output_to_file_cb &&
         "Invalid argument.");

  free(self->output.user_data);
}

bool ten_log_is_output_to_file(ten_log_t *self) {
  assert(self && "Invalid argument.");
  return self->output.output_cb == ten_log_output_to_file_cb;
}
