//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_utils/lib/atomic.h"

#include <stdint.h>

int64_t axis_atomic_load(volatile axis_atomic_t *a) {
  int64_t ret = 0;
  axis_memory_barrier();
  ret = *(a);
  axis_memory_barrier();
  return ret;
}

void axis_atomic_store(volatile axis_atomic_t *a, int64_t v) {
  axis_memory_barrier();
  *a = v;
  axis_memory_barrier();
}
