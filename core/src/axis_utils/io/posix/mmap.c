//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_utils/axis_config.h"

#include "axis_utils/io/mmap.h"

#include <errno.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>

#include "axis_utils/log/log.h"
#include "axis_utils/macro/check.h"

/**
 * @file This file implements file views and memory allocation when mmap is
 * available.
 */

bool axis_mmap_init(axis_mmap_t *self, int descriptor, off_t offset,
                   uint64_t size) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT((uint64_t)(size_t)size == size, "file size too large.");

  int pagesize = getpagesize();
  long in_page_offset = offset % pagesize;
  off_t offset_in_page_cnt = offset - in_page_offset;

  // Enlarge the size to the page boundary at the beginning and at the end.
  size += in_page_offset;
  size = (size + (pagesize - 1)) & ~(pagesize - 1);

  void *map =
      mmap(NULL, size, PROT_READ, MAP_PRIVATE, descriptor, offset_in_page_cnt);
  axis_ASSERT(map != MAP_FAILED, "Failed to mmap: %d", errno);

  self->data = (char *)map + in_page_offset;
  self->base = map;
  self->len = size;

  return true;
}

void axis_mmap_deinit(axis_mmap_t *self) {
  axis_ASSERT(self, "Invalid argument.");

  union {
    const void *cv;
    void *v;
  } const_cast;

  const_cast.cv = self->base;
  if (munmap(const_cast.v, self->len) < 0) {
    axis_LOGE("Failed to munmap: %d", errno);
  }
}
