//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_utils/lib/file.h"

#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "include_internal/axis_utils/log/log.h"
#include "axis_utils/lib/alloc.h"
#include "axis_utils/lib/path.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/macro/check.h"

#define FILE_COPY_BUF_SIZE 4096

int axis_file_remove(const char *filename) {
  axis_ASSERT(filename, "Invalid argument.");

  if (!filename || strlen(filename) == 0) {
    axis_ASSERT(filename, "Invalid argument.");
    return -1;
  }

  if (!axis_path_exists(filename)) {
    axis_LOGE("Failed to find %s", filename);
    return -1;
  }

  if (remove(filename)) {
    axis_LOGE("Failed to remove %s", filename);
    return -1;
  }

  return 0;
}

static char *axis_file_read_from_open_file(FILE *fp) {
  axis_ASSERT(fp, "Invalid argument.");

  if (fseek(fp, 0, SEEK_END) == -1) {
    axis_LOGE("Failed to fseek to the end of the file.");
    return NULL;
  }

  unsigned long length = ftell(fp);
  if (length <= 0) {
    axis_LOGW("File size is 0.");
    return NULL;
  }

  char *buf = (char *)axis_MALLOC(sizeof(char) * (length + 1));
  axis_ASSERT(buf, "Failed to allocate memory.");

  rewind(fp);

  length = fread(buf, 1, length, fp);
  buf[length] = '\0';

  return buf;
}

char *axis_file_read(const char *filename) {
  axis_ASSERT(filename, "Invalid argument.");

  if (!filename || strlen(filename) == 0) {
    axis_ASSERT(filename, "Invalid argument.");
    return NULL;
  }

  if (!axis_path_exists(filename)) {
    axis_LOGE("Failed to find %s", filename);
    return NULL;
  }

  FILE *file = fopen(filename, "rb");
  if (!file) {
    axis_LOGE("Failed to fopen %s", filename);
    return NULL;
  }

  char *buf = axis_file_read_from_open_file(file);

  if (fclose(file)) {
    axis_LOGE("Failed to fclose %s", filename);
  }

  return buf;
}

int axis_file_write(const char *filename, axis_buf_t buf) {
  axis_ASSERT(filename && buf.data, "Invalid argument.");

  if (!filename || strlen(filename) == 0) {
    axis_ASSERT(filename, "Invalid argument.");
    return -1;
  }

  FILE *file = fopen(filename, "wb");
  if (!file) {
    axis_LOGE("Failed to fopen %s", filename);
    return -1;
  }

  int result = axis_file_write_to_open_file(file, buf);

  if (fclose(file)) {
    axis_LOGE("Failed to fclose %s", filename);
  }

  return result;
}

int axis_file_write_to_open_file(FILE *fp, axis_buf_t buf) {
  axis_ASSERT(fp && buf.data, "Invalid argument.");

  size_t wrote_size = fwrite(buf.data, 1, buf.content_size, fp);
  if (wrote_size != buf.content_size) {
    axis_LOGE("Failed to write fwrite.");
    return -1;
  }

  return 0;
}

int axis_file_copy(const char *src_filename, const char *dest_filename) {
  axis_ASSERT(src_filename && dest_filename, "Invalid argument.");

  FILE *src_file = NULL;
  FILE *dest_file = NULL;
  char buffer[FILE_COPY_BUF_SIZE];
  unsigned long read_size = 0;
  unsigned long write_size = 0;
  int result = 0;

  src_file = fopen(src_filename, "rb");
  if (!src_file) {
    axis_LOGE("Failed to fopen source %s: %d", src_filename, errno);
    result = -1;
    goto error;
  }

  dest_file = fopen(dest_filename, "wb");
  if (!dest_file) {
    axis_LOGE("Failed to fopen destination %s: %d", dest_filename, errno);
    result = -1;
    goto error;
  }

  while ((read_size = fread(buffer, 1, FILE_COPY_BUF_SIZE, src_file)) > 0) {
    write_size = fwrite(buffer, 1, read_size, dest_file);

    if (write_size != read_size) {
      axis_LOGE("Failed to fwrite to %s", dest_filename);
      result = -1;
      break;
    }

    if (read_size < 0) {
      axis_LOGE("Failed to fread from %s", src_filename);
      result = -1;
      break;
    }
  }

  result = axis_file_clone_permission_by_fd(fileno(src_file), fileno(dest_file));

error:
  if (src_file) {
    if (fclose(src_file)) {
      axis_LOGE("Failed to fclose %s", src_filename);
    }
  }
  if (dest_file) {
    if (fclose(dest_file)) {
      axis_LOGE("Failed to fclose %s", dest_filename);
    }
  }
  return result;
}

int axis_file_copy_to_dir(const char *src_file, const char *dest_dir) {
  axis_ASSERT(src_file && dest_dir && axis_path_exists(dest_dir),
             "Invalid argument.");

  axis_string_t *src_file_axis_string =
      axis_string_create_formatted("%s", src_file);
  axis_string_t *filename = axis_path_get_filename(src_file_axis_string);
  axis_string_t *dest_file = axis_string_create_formatted(
      "%s/%s", dest_dir, axis_string_get_raw_str(filename));
  axis_path_to_system_flavor(dest_file);

  int rc = axis_file_copy(src_file, axis_string_get_raw_str(dest_file));

  axis_string_destroy(dest_file);
  axis_string_destroy(filename);
  axis_string_destroy(src_file_axis_string);

  return rc;
}
