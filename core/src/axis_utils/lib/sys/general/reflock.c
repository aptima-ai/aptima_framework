//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_utils/lib/reflock.h"

#include <assert.h>
#include <memory.h>

#include "axis_utils/lib/atomic.h"
#include "axis_utils/lib/event.h"
#include "axis_utils/macro/mark.h"

static void __reflock_signal_event(axis_reflock_t *lock) {
  axis_event_set(lock->event);
}

static void __reflock_await_event(axis_reflock_t *lock) {
  axis_event_wait(lock->event, -1);
}

void axis_reflock_init(axis_reflock_t *reflock) {
  reflock->state = 0;
  reflock->event = axis_event_create(0, 1);
}

void axis_reflock_ref(axis_reflock_t *reflock) {
  int64_t state = axis_atomic_add_fetch(&reflock->state, axis_REFLOCK_REF);

  /* Verify that the counter didn't overflow and the lock isn't destroyed. */
  assert((state & axis_REFLOCK_DESTROY_MASK) == 0);
  (void)(state);
}

void axis_reflock_unref(axis_reflock_t *reflock) {
  int64_t state = axis_atomic_sub_fetch(&reflock->state, axis_REFLOCK_REF);

  /* Verify that the lock was referenced and not already destroyed. */
  assert((state & axis_REFLOCK_DESTROY_MASK & ~axis_REFLOCK_DESTROY) == 0);
  if (UNLIKELY((state & axis_REFLOCK_DESTROY_MASK & ~axis_REFLOCK_DESTROY) !=
               0)) {
    return;
  }

  if (state == axis_REFLOCK_DESTROY) {
    __reflock_signal_event(reflock);
  }
}

void axis_reflock_unref_destroy(axis_reflock_t *reflock) {
  int64_t state = axis_atomic_add_fetch(&reflock->state,
                                       axis_REFLOCK_DESTROY - axis_REFLOCK_REF);
  int64_t ref_count = state & axis_REFLOCK_REF_MASK;

  /* Verify that the lock was referenced and not already destroyed. */
  assert((state & axis_REFLOCK_DESTROY_MASK) == axis_REFLOCK_DESTROY);
  if (UNLIKELY((state & axis_REFLOCK_DESTROY_MASK) != axis_REFLOCK_DESTROY)) {
    return;
  }

  if (ref_count != 0) {
    __reflock_await_event(reflock);
  }

  state = axis_atomic_test_set(&reflock->state, axis_REFLOCK_POISON);
  assert(state == axis_REFLOCK_DESTROY);

  axis_event_destroy(reflock->event);
  reflock->event = NULL;
}
