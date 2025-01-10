//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
// This file is modified from
// https://github.com/ianlancetaylor/libbacktrace [BSD license]
//
#include "include_internal/axis_utils/backtrace/platform/posix/linux/view.h"

#include <stdint.h>

#include "axis_utils/macro/check.h"
#include "axis_utils/macro/mark.h"

/**
 * @brief Create a view of SIZE bytes from DESCRIPTOR/MEMORY at OFFSET.
 */
int elf_get_view(axis_backtrace_t *self, int descriptor,
                 const unsigned char *memory, size_t memory_size, off_t offset,
                 uint64_t size, axis_backtrace_error_func_t error_cb, void *data,
                 struct elf_view *view) {
  axis_ASSERT(self, "Invalid argument.");

  if (memory == NULL) {
    view->release = 1;
    return axis_mmap_init(&view->view, descriptor, offset, size);
  } else {
    if ((uint64_t)offset + size > (uint64_t)memory_size) {
      error_cb(self, "out of range for in-memory file", 0, data);
      return 0;
    }
    view->view.data = (const void *)(memory + offset);
    view->view.base = NULL;
    view->view.len = size;
    view->release = 0;
    return 1;
  }
}

/**
 * @brief Release a view read by elf_get_view.
 */
void elf_release_view(axis_backtrace_t *self, struct elf_view *view,
                      axis_UNUSED axis_backtrace_error_func_t error_cb,
                      axis_UNUSED void *data) {
  axis_ASSERT(self, "Invalid argument.");

  if (view->release) {
    axis_mmap_deinit(&view->view);
  }
}
