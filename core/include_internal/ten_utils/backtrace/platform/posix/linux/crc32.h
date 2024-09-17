//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
// This file is modified from
// https://github.com/ianlancetaylor/libbacktrace [BSD license]
//
#include "ten_utils/ten_config.h"

#include "include_internal/ten_utils/backtrace/backtrace.h"

TEN_UTILS_PRIVATE_API uint32_t elf_crc32(uint32_t crc, const unsigned char *buf,
                                         size_t len);

TEN_UTILS_PRIVATE_API uint32_t
elf_crc32_file(ten_backtrace_t *self, int descriptor,
               ten_backtrace_error_func_t error_cb, void *data);
