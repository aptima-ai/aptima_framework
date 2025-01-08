//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
// This file is modified from
// https://github.com/ianlancetaylor/libbacktrace [BSD license]
//
#include "axis_utils/axis_config.h"

#include "include_internal/axis_utils/backtrace/backtrace.h"

axis_UTILS_PRIVATE_API uint32_t elf_crc32(uint32_t crc, const unsigned char *buf,
                                         size_t len);

axis_UTILS_PRIVATE_API uint32_t
elf_crc32_file(axis_backtrace_t *self, int descriptor,
               axis_backtrace_error_func_t error_cb, void *data);
