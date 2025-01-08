//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_utils/axis_config.h"

#include "axis_utils/lib/string.h"

/**
 * @brief Get current working directory.
 * @return Current working directory.
 */
axis_UTILS_API axis_string_t *axis_path_get_cwd(void);

/**
 * @brief Get current user's home directory.
 * @return Current user's home directory.
 */
axis_UTILS_API axis_string_t *axis_path_get_home_path(void);

/**
 * @brief Get directory of current module
 * @param addr The address of which it belong to
 * @return Directory of current module
 */
axis_UTILS_API axis_string_t *axis_path_get_module_path(const void *addr);

/**
 * @brief Get directory of executable binary
 * @return Directory of current executable binary
 */
axis_UTILS_API axis_string_t *axis_path_get_executable_path(void);

/**
 * @brief Convert a path into system preferred format.
 *        For example on Windows the path "c:/aaa/bbb" will be
 *        converted to "C:\\aaa\\bbb".
 * @param path Path to be converted.
 * @return 0 if success, -1 if failed.
 */
axis_UTILS_API int axis_path_to_system_flavor(axis_string_t *path);

/**
 * @brief Join two path.
 *        For example (/aaa/bbb, ccc) -> /aaa/bbb/ccc
 * @return 0 if success, -1 if failed.
 */
axis_UTILS_API int axis_path_join_c_str(axis_string_t *self, const char *other);

/**
 * @brief Get dir of a path.
 *        For example /aaa/bbb -> /aaa
 * @param path Path of which dir is to be got.
 * @return Dir of the path.
 */
axis_UTILS_API axis_string_t *axis_path_get_dirname(const axis_string_t *path);

/**
 * @brief Get file extension of a path.
 *        For example /aaa/bbb.zip -> .zip
 * @param path Path of which file extension is to be got.
 * @return File extension of the path.
 */
axis_UTILS_API axis_string_t *axis_path_get_extension(const axis_string_t *path);

/**
 * @brief Get file name of a path.
 *        For example /aaa/bbb -> bbb
 *                    /aaa/bbb/ -> <empty>
 * @param path Path of which file name is to be got.
 * @return File name of the path.
 */
axis_UTILS_API axis_string_t *axis_path_get_filename(const axis_string_t *path);

/**
 * @brief Get last part of a path.
 *        For example /aaa/bbb -> bbb
 *                    /aaa/bbb/ -> bbb
 * @param path Path of which last part is to be got.
 * @return Last part of the path.
 */
axis_UTILS_API axis_string_t *axis_path_get_last_part(const axis_string_t *path);

/**
 * @brief Get absolute path of a path.
 *        For example /aaa/./../aaa/bbb -> /aaa/bbb
 * @param path Path of which absolute path is to be got.
 * @return Absolute path of the path.
 */
axis_UTILS_API axis_string_t *axis_path_realpath(const axis_string_t *path);

/**
 * @brief Check whether a path is a dir
 * @param path Path to be checked.
 * @return 1 if path is a dir, 0 if not.
 */
axis_UTILS_API int axis_path_is_dir(const axis_string_t *path);

/**
 * @brief Check whether a path ends with "." or ".."
 * @param path Path to be checked.
 * @return 1 if path ends with  "." or "..", 0 if not.
 */
axis_UTILS_API int axis_path_is_special_dir(const axis_string_t *path);

/**
 * @brief Check whether a path is a a shared library
 * @param path Path to be checked.
 * @return 1 if path is a shared library, 0 if not.
 */
axis_UTILS_API int axis_path_is_shared_library(const axis_string_t *path);

/**
 * @brief Make dirs for a path
 * @param path Path to be made.
 * @param recursive Whether to make dirs recursively.
 * @return 0 if success, -1 if failed.
 */
axis_UTILS_API int axis_path_mkdir(const axis_string_t *path, int recursive);

axis_UTILS_API int axis_path_create_temp_dir(const char *base_path,
                                           axis_string_t *tmp_dir_path);

/**
 * @brief Check whether a path exists
 * @param path Path to be checked.
 * @return 1 if path exists, 0 if not.
 */
axis_UTILS_API int axis_path_exists(const char *path);

typedef struct axis_path_itor_t axis_path_itor_t;

typedef struct axis_dir_fd_t axis_dir_fd_t;

/**
 * @brief Open a path for iteration
 * @param path Path to be opened.
 * @return Iterator of the path. NULL if error happens
 */
axis_UTILS_API axis_dir_fd_t *axis_path_open_dir(const char *path);

/**
 * @brief Close an iterator of a path
 * @param dir Iterator to be closed.
 * @return 0 if success, -1 if failed.
 */
axis_UTILS_API int axis_path_close_dir(axis_dir_fd_t *dir);

/**
 * @brief Get the first entry of an iterator
 * @param dir Iterator to be iterated.
 * @return The first entry of the iterator. NULL if error happens
 */
axis_UTILS_API axis_path_itor_t *axis_path_get_first(axis_dir_fd_t *dir);

/**
 * @brief Get the next entry of an iterator
 * @param dir Iterator to be iterated.
 * @return The next entry of the iterator. NULL if no more entries
 */
axis_UTILS_API axis_path_itor_t *axis_path_get_next(axis_path_itor_t *itor);

/**
 * @brief Get the short name of an entry
 * @param itor Entry to be got.
 * @return The short name of the entry. NULL if error happens
 */
axis_UTILS_API axis_string_t *axis_path_itor_get_name(axis_path_itor_t *itor);

/**
 * @brief Get the full name of an entry
 * @param itor Entry to be got.
 * @return The full name of the entry. NULL if error happens
 */
axis_UTILS_API axis_string_t *axis_path_itor_get_full_name(axis_path_itor_t *itor);

/**
 * @brief Change current working directory.
 */
axis_UTILS_API int axis_path_change_cwd(axis_string_t *dirname);

/**
 * @brief Check if the parameter is a absolute path or not.
 */
axis_UTILS_API int axis_path_is_absolute(const axis_string_t *path);

/**
 * @brief Make a symbolic link (also known as a soft link) to a target.
 * @return 0 if success, -1 if failed.
 */
axis_UTILS_API int axis_path_make_symlink(const char *target,
                                        const char *link_path);

/**
 * @brief Check if the path is a symbolic link.
 * @return 1 if path is symlink, 0 if not.
 */
axis_UTILS_API int axis_path_is_symlink(const char *path);
