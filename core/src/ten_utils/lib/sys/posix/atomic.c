//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include "ten_utils/lib/atomic.h"

#include <stdbool.h>

// TODO(Wei): Consider to lower the memory ordering constraints parameter from
// __ATOMIC_SEQ_CST to a lighter mode.

int64_t ten_atomic_fetch_add(volatile ten_atomic_t *a, int64_t v) {
  return __atomic_fetch_add(a, v, __ATOMIC_SEQ_CST);
}

int64_t ten_atomic_fetch_and(volatile ten_atomic_t *a, int64_t v) {
  return __atomic_fetch_and(a, v, __ATOMIC_SEQ_CST);
}

int64_t ten_atomic_fetch_or(volatile ten_atomic_t *a, int64_t v) {
  return __atomic_fetch_or(a, v, __ATOMIC_SEQ_CST);
}

int64_t ten_atomic_add_fetch(volatile ten_atomic_t *a, int64_t v) {
  return __atomic_add_fetch(a, v, __ATOMIC_SEQ_CST);
}

int64_t ten_atomic_and_fetch(volatile ten_atomic_t *a, int64_t v) {
  return __atomic_and_fetch(a, v, __ATOMIC_SEQ_CST);
}

int64_t ten_atomic_fetch_sub(volatile ten_atomic_t *a, int64_t v) {
  return __atomic_fetch_sub(a, v, __ATOMIC_SEQ_CST);
}

int64_t ten_atomic_sub_fetch(volatile ten_atomic_t *a, int64_t v) {
  return __atomic_sub_fetch(a, v, __ATOMIC_SEQ_CST);
}

int64_t ten_atomic_or_fetch(volatile ten_atomic_t *a, int64_t v) {
  return __atomic_or_fetch(a, v, __ATOMIC_SEQ_CST);
}

int64_t ten_atomic_test_set(volatile ten_atomic_t *a, int64_t v) {
  return __atomic_exchange_n(a, v, __ATOMIC_SEQ_CST);
}

int ten_atomic_bool_compare_swap(volatile ten_atomic_t *a, int64_t comp,
                                 int64_t xchg) {
  return __atomic_compare_exchange_n(a, &comp, xchg, false, __ATOMIC_SEQ_CST,
                                     __ATOMIC_SEQ_CST);
}

int64_t ten_atomic_inc_if_non_zero(volatile ten_atomic_t *a) {
  int64_t r = ten_atomic_load(a);

  for (;;) {
    if (r == 0) {
      return r;
    }

    if (__atomic_compare_exchange_n(a, &r, r + 1, false, __ATOMIC_SEQ_CST,
                                    __ATOMIC_SEQ_CST)) {
      return r;
    }
  }
}

int64_t ten_atomic_dec_if_non_zero(volatile ten_atomic_t *a) {
  int64_t r = ten_atomic_load(a);

  for (;;) {
    if (r == 0) {
      return r;
    }

    if (__atomic_compare_exchange_n(a, &r, r - 1, false, __ATOMIC_SEQ_CST,
                                    __ATOMIC_SEQ_CST)) {
      return r;
    }
  }
}
