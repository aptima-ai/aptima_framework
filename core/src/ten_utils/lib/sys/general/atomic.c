//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include "ten_utils/lib/atomic.h"

#include <stdint.h>

int64_t ten_atomic_load(volatile ten_atomic_t *a) {
  int64_t ret = 0;
  ten_memory_barrier();
  ret = *(a);
  ten_memory_barrier();
  return ret;
}

void ten_atomic_store(volatile ten_atomic_t *a, int64_t v) {
  ten_memory_barrier();
  *a = v;
  ten_memory_barrier();
}
