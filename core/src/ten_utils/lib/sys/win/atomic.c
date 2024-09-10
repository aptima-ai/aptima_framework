//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include "ten_utils/lib/atomic.h"

#include <Windows.h>
#include <assert.h>

#include "ten_utils/macro/check.h"

int64_t ten_atomic_fetch_add(volatile ten_atomic_t *a, int64_t v) {
  return InterlockedExchangeAdd64(a, v);
}

int64_t ten_atomic_add_fetch(volatile ten_atomic_t *a, int64_t v) {
  return InterlockedAddAcquire64(a, v);
}

int64_t ten_atomic_and_fetch(volatile ten_atomic_t *a, int64_t v) {
  TEN_ASSERT(0, "TODO(Wei): Implement this.");
  return 0;
}

int64_t ten_atomic_or_fetch(volatile ten_atomic_t *a, int64_t v) {
  TEN_ASSERT(0, "TODO(Wei): Implement this.");
  return 0;
}

int64_t ten_atomic_fetch_sub(volatile ten_atomic_t *a, int64_t v) {
  return InterlockedExchangeAdd64(a, (0 - v));
}

int64_t ten_atomic_sub_fetch(volatile ten_atomic_t *a, int64_t v) {
  return InterlockedAddAcquire64(a, (0 - v));
}

int64_t ten_atomic_test_set(volatile ten_atomic_t *a, int64_t v) {
  return InterlockedExchange64(a, v);
}

int ten_atomic_bool_compare_swap(volatile ten_atomic_t *a, int64_t comp,
                                 int64_t xchg) {
  return InterlockedCompareExchange64(a, xchg, comp) == comp;
}

int64_t ten_atomic_inc_if_non_zero(volatile ten_atomic_t *a) {
  int64_t r = ten_atomic_load(a);

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

int64_t ten_atomic_dec_if_non_zero(volatile ten_atomic_t *a) {
  int64_t r = ten_atomic_load(a);

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

int64_t ten_atomic_fetch_and(volatile ten_atomic_t *a, int64_t v) {
  return InterlockedAnd64(a, v);
}

int64_t ten_atomic_fetch_or(volatile ten_atomic_t *a, int64_t v) {
  return InterlockedOr64(a, v);
}

void ten_memory_barrier(void) { MemoryBarrier(); }
