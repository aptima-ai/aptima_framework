//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include "ten_utils/lib/thread.h"

void ten_thread_pause_cpu(void) { asm volatile("yield" ::: "memory"); }