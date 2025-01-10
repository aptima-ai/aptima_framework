//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
// This file is modified from
// https://github.com/ianlancetaylor/libbacktrace [BSD license]
//
#include "axis_utils/axis_config.h"

#include "include_internal/axis_utils/backtrace/backtrace.h"

axis_UTILS_PRIVATE_API int elf_open_debug_file_by_build_id(
    axis_backtrace_t *self, const char *build_id_data, size_t build_id_size);

axis_UTILS_PRIVATE_API int elf_open_debug_file_by_debug_link(
    axis_backtrace_t *self, const char *filename, const char *debug_link_name,
    uint32_t debug_link_crc, axis_backtrace_error_func_t error_cb, void *data);
