//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
// This file is modified from
// https://github.com/ianlancetaylor/libbacktrace [BSD license]
//
#pragma once

#include "axis_utils/axis_config.h"

#include "include_internal/axis_utils/backtrace/backtrace.h"
#include "include_internal/axis_utils/backtrace/common.h"
#include "axis_utils/lib/atomic.h"
#include "axis_utils/macro/mark.h"

#ifndef __GNUC__
#define __builtin_prefetch(p, r, l)
#define unlikely(x) (x)
#else
#define unlikely(x) __builtin_expect(!!(x), 0)
#endif

/**
 * @brief The type of the function that collects file/line information.
 */
typedef int (*axis_backtrace_get_file_line_func_t)(
    axis_backtrace_t *self, uintptr_t pc, axis_backtrace_dump_file_line_func_t cb,
    axis_backtrace_error_func_t error_cb, void *data);

/**
 * @brief The type of the function that collects symbol information.
 */
typedef void (*axis_axis_backtrace_get_syminfo_func_t)(
    axis_backtrace_t *self, uintptr_t pc, axis_backtrace_dump_syminfo_func_t cb,
    axis_backtrace_error_func_t error_cb, void *data);

/**
 * @brief The posix specific axis_backtrace_t implementation.
 */
typedef struct axis_backtrace_posix_t {
  axis_backtrace_common_t common;

  // The function that returns file/line information.
  axis_backtrace_get_file_line_func_t get_file_line;
  // The data to pass to 'file_line_fn'.
  void *get_file_line_data;

  // The function that returns symbol information.
  axis_axis_backtrace_get_syminfo_func_t get_syminfo;
  // The data to pass to 'syminfo_fn'.
  void *get_syminfo_data;

  // Whether initializing the file/line information failed.
  axis_atomic_t file_line_init_failed;
} axis_backtrace_posix_t;

axis_UTILS_PRIVATE_API int axis_backtrace_dump_posix(axis_backtrace_t *self,
                                                   size_t skip);

/**
 * @brief Sort without using memory.
 */
axis_UTILS_PRIVATE_API void backtrace_qsort(void *base, size_t count,
                                           size_t size,
                                           int (*compar)(const void *,
                                                         const void *));

/**
 * @brief Read initial debug data from a descriptor, and set the the following
 * fields of @a self.
 * - get_file_line_data
 * - get_syminfo
 * - get_syminfo_data
 *
 * @param get_file_line The value of the get_file_line field of @a self.
 * @return 1 on success, 0 on error.
 *
 * @note This function is called after the descriptor has first been opened.
 * It will close the descriptor if it is no longer needed.
 */
axis_UTILS_PRIVATE_API int axis_backtrace_init_posix(
    axis_backtrace_t *self, const char *filename, int descriptor,
    axis_backtrace_error_func_t error_cb, void *data,
    axis_backtrace_get_file_line_func_t *get_file_line);

/**
 * @brief An enum for the DWARF sections we care about.
 */
typedef enum dwarf_section {
  DEBUG_INFO,
  DEBUG_LINE,
  DEBUG_ABBREV,
  DEBUG_RANGES,
  DEBUG_STR,
  DEBUG_ADDR,
  DEBUG_STR_OFFSETS,
  DEBUG_LINE_STR,
  DEBUG_RNGLISTS,

  DEBUG_MAX
} dwarf_section;

// Data for the DWARF sections we care about.
typedef struct dwarf_sections {
  const unsigned char *data[DEBUG_MAX];
  size_t size[DEBUG_MAX];
} dwarf_sections;

// DWARF data read from a file, used for .gnu_debugaltlink.
struct dwarf_data;

/**
 * @brief Add file/line information for a DWARF module.
 */
axis_UTILS_PRIVATE_API int backtrace_dwarf_add(
    axis_backtrace_t *self, uintptr_t base_address,
    const struct dwarf_sections *dwarf_sections, int is_bigendian,
    struct dwarf_data *fileline_altlink, axis_backtrace_error_func_t error_cb,
    void *data, axis_backtrace_get_file_line_func_t *fileline_fn,
    struct dwarf_data **fileline_entry);

// A data structure to pass to backtrace_syminfo_to_full.
struct backtrace_call_full {
  axis_backtrace_dump_file_line_func_t dump_file_line_cb;
  axis_backtrace_error_func_t error_cb;
  void *data;
  int ret;
};

/**
 * @brief A axis_backtrace_dump_syminfo_func_t that can call into a
 * axis_backtrace_dump_file_line_func_t, used when we have a symbol table but no
 * debug info.
 */
axis_UTILS_PRIVATE_API void backtrace_dump_syminfo_to_dump_file_line_cb(
    axis_backtrace_t *self, uintptr_t pc, const char *symname,
    axis_UNUSED uintptr_t sym_val, axis_UNUSED uintptr_t sym_size, void *data);

/**
 * @brief An error callback that corresponds to
 * backtrace_dump_syminfo_to_dump_file_line.
 */
axis_UTILS_PRIVATE_API void backtrace_dump_syminfo_to_dump_file_line_error_cb(
    axis_backtrace_t *self, const char *msg, int errnum, void *data);
