//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_utils/lib/shared_event.h"

#include <memory.h>
#include <stdlib.h>

#include "axis_utils/lib/atomic.h"
#include "axis_utils/lib/spinlock.h"
#include "axis_utils/lib/time.h"
#include "axis_utils/lib/waitable_addr.h"

struct axis_shared_event_t {
  axis_waitable_t *signal;
  axis_spinlock_t *lock;
  int auto_reset;
};

axis_shared_event_t *axis_shared_event_create(uint32_t *addr_for_sig,
                                            axis_atomic_t *addr_for_lock,
                                            int init_state, int auto_reset) {
  axis_shared_event_t *ret = NULL;

  if (!addr_for_sig || !addr_for_lock) {
    return NULL;
  }

  ret = (axis_shared_event_t *)malloc(sizeof(*ret));
  if (!ret) {
    return NULL;
  }

  ret->signal = axis_waitable_from_addr(addr_for_sig);
  ret->lock = axis_spinlock_from_addr(addr_for_lock);
  ret->signal->sig = init_state;
  ret->auto_reset = auto_reset;
  return ret;
}

int axis_shared_event_wait(axis_shared_event_t *event, int wait_ms) {
  int64_t timeout_time = 0;
  uint64_t loops = 0;

  if (!event || !event->signal || !event->lock) {
    return -1;
  }

  axis_spinlock_lock(event->lock);
  if (wait_ms == 0) {
    if (!axis_waitable_get(event->signal)) {
      axis_spinlock_unlock(event->lock);
      return -1;
    }

    if (event->auto_reset) {
      axis_waitable_set(event->signal, 0);
    }

    axis_spinlock_unlock(event->lock);
    return 0;
  }

  timeout_time = wait_ms < 0 ? -1 : axis_current_time() + wait_ms;
  while (!axis_waitable_get(event->signal)) {
    int64_t diff = -1;

    if (wait_ms > 0) {
      diff = timeout_time - axis_current_time();

      if (timeout_time > 0 && diff < 0) {
        return -1;
      }
    }

    if (loops++ > 200) {
      continue;
    }

    if (axis_waitable_wait(event->signal, 0, event->lock, (int)diff) != 0) {
      return -1;
    }
  }

  if (event->auto_reset) {
    event->signal = 0;
  }

  axis_spinlock_unlock(event->lock);
  return 0;
}

void axis_shared_event_set(axis_shared_event_t *event) {
  if (!event || !event->signal || !event->lock) {
    return;
  }

  axis_spinlock_lock(event->lock);
  axis_waitable_set(event->signal, 1);
  if (event->auto_reset) {
    axis_waitable_notify(event->signal);
  } else {
    axis_waitable_notify_all(event->signal);
  }
  axis_spinlock_unlock(event->lock);
}

void axis_shared_event_reset(axis_shared_event_t *event) {
  if (!event || !event->signal || !event->lock || event->auto_reset) {
    return;
  }

  axis_spinlock_lock(event->lock);
  axis_waitable_set(event->signal, 0);
  axis_spinlock_unlock(event->lock);
}

void axis_shared_event_destroy(axis_shared_event_t *event) { free(event); }
