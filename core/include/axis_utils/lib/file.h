//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_utils/axis_config.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "axis_utils/lib/buf.h"

typedef struct axis_string_t axis_string_t;

axis_UTILS_API int axis_file_remove(const char *filename);

axis_UTILS_API int axis_file_size(const char *filename);

axis_UTILS_API char *axis_file_read(const char *filename);

axis_UTILS_API char *axis_symlink_file_read(const char *path);

axis_UTILS_API int axis_file_write(const char *filename, axis_buf_t buf);

axis_UTILS_API int axis_file_write_to_open_file(FILE *fp, axis_buf_t buf);

axis_UTILS_API int axis_file_clear_open_file_content(FILE *fp);

axis_UTILS_API int axis_file_copy(const char *src_filename,
                                const char *dest_filename);

axis_UTILS_API int axis_file_copy_to_dir(const char *src_file,
                                       const char *dest_dir);

axis_UTILS_API int axis_symlink_file_copy(const char *src_file,
                                        const char *dest_file);

axis_UTILS_API int axis_file_get_fd(FILE *fp);

axis_UTILS_API int axis_file_chmod(const char *filename, uint32_t mode);

axis_UTILS_API int axis_file_clone_permission(const char *src_filename,
                                            const char *dest_filename);

axis_UTILS_API int axis_file_clone_permission_by_fd(int src_fd, int dest_fd);

/**
 * @brief Open a file for reading.
 *
 * @param does_not_exist If @a does_not_exist is not NULL, @a *does_not_exist
 * will be set to false normally and set to true if the file does not exist. If
 * the file does not exist and @a does_not_exist is not NULL, the function will
 * return -1.
 *
 * @return -1 on error.
 */
axis_UTILS_API int axis_file_open(const char *filename, bool *does_not_exist);

/**
 * @brief Close a file opened by axis_file_open().
 *
 * @return true on success, false on error.
 */
axis_UTILS_API bool axis_file_close(int fd);
