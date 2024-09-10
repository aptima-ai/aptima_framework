//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include "ten_utils/lib/waitable_addr.h"

#include <memory.h>

#include "ten_utils/lib/atomic.h"

uint32_t ten_waitable_get(ten_waitable_t *wb) {
  uint32_t ret = 0;

  ten_memory_barrier();
  ret = wb->sig;
  ten_memory_barrier();
  return ret;
}

void ten_waitable_set(ten_waitable_t *wb, uint32_t val) {
  ten_memory_barrier();
  wb->sig = val;
  ten_memory_barrier();
}
