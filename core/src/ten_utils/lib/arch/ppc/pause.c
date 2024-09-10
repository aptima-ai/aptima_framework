//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include "ten_utils/lib/atomic.h"
#include "ten_utils/lib/thread.h"

void ten_thread_pause_cpu(void) {
  /* Set hardware multi-threading low priority */
  asm volatile("or 1,1,1");
  /* Set hardware multi-threading medium priority */
  asm volatile("or 2,2,2");
  ten_compiler_barrier();
}