//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include "ten_utils/ten_config.h"

#include "ten_utils/io/mmap.h"

#include <errno.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>

#include "ten_utils/macro/check.h"

/**
 * @file This file implements file views and memory allocation when mmap is
 * available.
 */

bool ten_mmap_init(ten_mmap_t *self, int descriptor, off_t offset,
                   uint64_t size) {
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT((uint64_t)(size_t)size == size, "file size too large.");

  int pagesize = getpagesize();
  long in_page_offset = offset % pagesize;
  off_t offset_in_page_cnt = offset - in_page_offset;

  // Enlarge the size to the page boundary at the beginning and at the end.
  size += in_page_offset;
  size = (size + (pagesize - 1)) & ~(pagesize - 1);

  void *map =
      mmap(NULL, size, PROT_READ, MAP_PRIVATE, descriptor, offset_in_page_cnt);
  TEN_ASSERT(map != MAP_FAILED, "Failed to mmap: %d", errno);

  self->data = (char *)map + in_page_offset;
  self->base = map;
  self->len = size;

  return true;
}

void ten_mmap_deinit(ten_mmap_t *self) {
  TEN_ASSERT(self, "Invalid argument.");

  union {
    const void *cv;
    void *v;
  } const_cast;

  const_cast.cv = self->base;
  if (munmap(const_cast.v, self->len) < 0) {
    TEN_LOGE("Failed to munmap: %d", errno);
  }
}
