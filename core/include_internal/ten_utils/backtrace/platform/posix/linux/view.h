//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
// This file is modified from
// https://github.com/ianlancetaylor/libbacktrace [BSD license]
//
#include "ten_utils/ten_config.h"

#include <stdint.h>

#include "include_internal/ten_utils/backtrace/backtrace.h"
#include "ten_utils/io/mmap.h"

/**
 * @brief A view that works for either a file or memory.
 */
struct elf_view {
  ten_mmap_t view;
  int release;  // If non-zero, must call ten_mmap_deinit.
};

TEN_UTILS_PRIVATE_API int elf_get_view(ten_backtrace_t *self, int descriptor,
                                       const unsigned char *memory,
                                       size_t memory_size, off_t offset,
                                       uint64_t size,
                                       ten_backtrace_error_func_t error_cb,
                                       void *data, struct elf_view *view);

TEN_UTILS_PRIVATE_API void elf_release_view(ten_backtrace_t *self,
                                            struct elf_view *view,
                                            ten_backtrace_error_func_t error_cb,
                                            void *data);
