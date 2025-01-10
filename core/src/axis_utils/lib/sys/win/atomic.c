//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_utils/lib/atomic.h"

#include <Windows.h>
#include <assert.h>

#include "axis_utils/macro/check.h"

int64_t axis_atomic_fetch_add(volatile axis_atomic_t *a, int64_t v) {
  return InterlockedExchangeAdd64(a, v);
}

int64_t axis_atomic_add_fetch(volatile axis_atomic_t *a, int64_t v) {
  return InterlockedAddAcquire64(a, v);
}

int64_t axis_atomic_and_fetch(volatile axis_atomic_t *a, int64_t v) {
  axis_ASSERT(0, "TODO(Wei): Implement this.");
  return 0;
}

int64_t axis_atomic_or_fetch(volatile axis_atomic_t *a, int64_t v) {
  axis_ASSERT(0, "TODO(Wei): Implement this.");
  return 0;
}

int64_t axis_atomic_fetch_sub(volatile axis_atomic_t *a, int64_t v) {
  return InterlockedExchangeAdd64(a, (0 - v));
}

int64_t axis_atomic_sub_fetch(volatile axis_atomic_t *a, int64_t v) {
  return InterlockedAddAcquire64(a, (0 - v));
}

int64_t axis_atomic_test_set(volatile axis_atomic_t *a, int64_t v) {
  return InterlockedExchange64(a, v);
}

int axis_atomic_bool_compare_swap(volatile axis_atomic_t *a, int64_t comp,
                                 int64_t xchg) {
  return InterlockedCompareExchange64(a, xchg, comp) == comp;
}

int64_t axis_atomic_inc_if_non_zero(volatile axis_atomic_t *a) {
  int64_t r = axis_atomic_load(a);

  for (;;) {
    if (r == 0) {
      return r;
    }

    int64_t got = InterlockedCompareExchange64(a, r + 1, r);
    if (got == r) {
      return r;
    }
    r = got;
  }
}

int64_t axis_atomic_dec_if_non_zero(volatile axis_atomic_t *a) {
  int64_t r = axis_atomic_load(a);

  for (;;) {
    if (r == 0) {
      return r;
    }

    int64_t got = InterlockedCompareExchange64(a, r - 1, r);
    if (got == r) {
      return r;
    }
    r = got;
  }
}

int64_t axis_atomic_fetch_and(volatile axis_atomic_t *a, int64_t v) {
  return InterlockedAnd64(a, v);
}

int64_t axis_atomic_fetch_or(volatile axis_atomic_t *a, int64_t v) {
  return InterlockedOr64(a, v);
}

void axis_memory_barrier(void) { MemoryBarrier(); }
