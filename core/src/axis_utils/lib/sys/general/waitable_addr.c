//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_utils/lib/waitable_addr.h"

#include <memory.h>

#include "axis_utils/lib/atomic.h"

uint32_t axis_waitable_get(axis_waitable_t *wb) {
  uint32_t ret = 0;

  axis_memory_barrier();
  ret = wb->sig;
  axis_memory_barrier();
  return ret;
}

void axis_waitable_set(axis_waitable_t *wb, uint32_t val) {
  axis_memory_barrier();
  wb->sig = val;
  axis_memory_barrier();
}
