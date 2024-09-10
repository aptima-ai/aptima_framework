//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include "ten_utils/lib/atomic.h"

void ten_memory_barrier(void) { asm volatile("sync" : : : "memory"); }