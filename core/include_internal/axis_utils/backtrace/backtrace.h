//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
// This file is modified from
// https://github.com/ianlancetaylor/libbacktrace [BSD license]
//
#pragma once

#include "axis_utils/axis_config.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct axis_backtrace_t axis_backtrace_t;

/**
 * @brief The type of the callback to be called when one backtrace frame is
 * dumping.
 *
 * @param axis_backtrace
 * @param pc The program counter.
 * @param filename The name of the file containing @a pc, or NULL if not
 *                 available.
 * @param LINENO The line number in @a filename containing @a pc, or 0 if not
 *               available.
 * @param function The name of the function containing @a pc, or NULL if
 *                 not available.
 *
 * @return 0 to continuing tracing.
 *
 * @note The @a filename and @a function buffers may become invalid after this
 * function returns.
 */
typedef int (*axis_backtrace_dump_file_line_func_t)(
    axis_backtrace_t *axis_backtrace, uintptr_t pc, const char *filename,
    int lineno, const char *function, void *data);

/**
 * @brief The type of the callback to be called when one symbol information is
 * dumping.
 *
 * @param axis_backtrace
 * @param pc The program counter.
 * @param sym_name The name of the symbol for the corresponding code.
 * @param sym_val The value of the symbol.
 * @param sym_size The size of the symbol.
 *
 * @note @a symname will be NULL if no error occurred but the symbol could not
 * be found.
 */
typedef void (*axis_backtrace_dump_syminfo_func_t)(
    axis_backtrace_t *axis_backtrace, uintptr_t pc, const char *sym_name,
    uintptr_t sym_val, uintptr_t sym_size, void *data);

/**
 * @brief The type of the error callback to be called when backtrace module
 * encounters errors. This function, if not NULL, will be called for certain
 * error cases.
 *
 * @param axis_backtrace
 * @param msg An error message.
 * @param errnum if greater than 0, holds an errno value.
 *
 * @note The @a msg buffer may become invalid after this function returns.
 * @note As a special case, the @a errnum argument will be passed as -1 if no
 * debug info can be found for the executable, or if the debug info exists but
 * has an unsupported version, but the function requires debug info (e.g.,
 * axis_backtrace_dump). The @a msg in this case will be something along the
 * lines of "no debug info".
 * @note Similarly, @a errnum will be passed as -1 if there is no symbol table,
 * but the function requires a symbol table (e.g., backtrace_syminfo). This may
 * be used as a signal that some other approach should be tried.
 */
typedef void (*axis_backtrace_error_func_t)(axis_backtrace_t *self,
                                           const char *msg, int errnum,
                                           void *data);

/**
 * @brief Given @a pc, a program counter in the current program, call the
 * @a dump_file_line_cb function with filename, line number, and function name
 * information. This will normally call the callback function exactly once.
 * However, if the @a pc happens to describe an inlined call, and the debugging
 * information contains the necessary information, then this may call the
 * callback function multiple times. This will make at least one call to either
 * @a dump_file_line_cb or @a error_cb.
 *
 * @return The first non-zero value returned by @a dump_file_line_cb or @a
 * error_cb, or 0.
 */
axis_UTILS_API int axis_backtrace_get_file_line_info(
    axis_backtrace_t *self, uintptr_t pc,
    axis_backtrace_dump_file_line_func_t dump_file_line_cb,
    axis_backtrace_error_func_t error_cb, void *data);

/**
 * @brief Given @a pc, an address or program counter in the current program,
 * call the callback information with the symbol name and value describing the
 * function or variable in which @a pc may be found.
 * This will call either @a dump_syminfo_cb or @a error_cb exactly once.
 *
 * @return 1 on success, 0 on failure.
 *
 * @note This function requires the symbol table but does not require the debug
 * info. Note that if the symbol table is present but @a pc could not be found
 * in the table, @a dump_syminfo_cb will be called with a NULL @a sym_name
 * argument. Returns 1 on success, 0 on error.
 */
axis_UTILS_API int axis_backtrace_get_syminfo(
    axis_backtrace_t *self, uintptr_t pc,
    axis_backtrace_dump_syminfo_func_t dump_syminfo_cb,
    axis_backtrace_error_func_t error_cb, void *data);

axis_UTILS_API void axis_backtrace_create_global(void);

axis_UTILS_API axis_backtrace_t *axis_backtrace_create(void);

axis_UTILS_API void axis_backtrace_destroy_global(void);

axis_UTILS_API void axis_backtrace_destroy(axis_backtrace_t *self);

/**
 * @brief Get a full stack backtrace.
 *
 * @param skip The number of frames to skip. Passing 0 will start the trace with
 * the function calling axis_backtrace_dump.
 *
 * @note If any call to 'dump' callback returns a non-zero value, the stack
 * backtrace stops, and backtrace returns that value; this may be used to
 * limit the number of stack frames desired.
 * @note If all calls to 'dump' callback return 0, backtrace returns 0. The
 * axis_backtrace_dump function will make at least one call to either
 * 'dump' callback or 'error' callback.
 * @note This function requires debug info for the executable.
 */
axis_UTILS_API void axis_backtrace_dump(axis_backtrace_t *self, size_t skip);

#ifdef __cplusplus
} /* End extern "C".  */
#endif
