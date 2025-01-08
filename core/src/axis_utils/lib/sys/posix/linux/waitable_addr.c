//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_utils/axis_config.h"

#include "axis_utils/lib/waitable_addr.h"

#include <errno.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <unistd.h>

#include "axis_utils/lib/atomic.h"
#include "axis_utils/lib/spinlock.h"
#include "axis_utils/lib/thread_once.h"
#include "axis_utils/lib/time.h"

static axis_thread_once_t g_detect = axis_THREAD_ONCE_INIT;
static axis_atomic_t g_futex_supported = 0;
const static int FUTEX_WAIT_OP = 0;
const static int FUTEX_WAKE_OP = 1;

axis_UTILS_API int __busy_loop(volatile uint32_t *addr, uint32_t expect,
                              axis_spinlock_t *lock, int timeout);

static int __futex(int *uaddr, int futex_op, int val,
                   const struct timespec *timeout) {
  return syscall(SYS_futex, uaddr, futex_op, val, timeout, NULL, 0);
}

static void __detect_futex_callback() {
  int test_value = 0;
  __futex(&test_value, FUTEX_WAIT_OP, 10, NULL);
  axis_atomic_store(&g_futex_supported, (errno != ENOSYS));
}

void axis_waitable_init(axis_waitable_t *wb) {
  static const axis_waitable_t initializer = axis_WAITABLE_INIT;

  axis_thread_once(&g_detect, __detect_futex_callback);
  *wb = initializer;
}

axis_waitable_t *axis_waitable_from_addr(uint32_t *address) {
  axis_waitable_t *ret = (axis_waitable_t *)address;

  if (!address) {
    return NULL;
  }

  axis_waitable_init(ret);
  return ret;
}

int axis_waitable_wait(axis_waitable_t *wb, uint32_t expect, axis_spinlock_t *lock,
                      int timeout) {
  struct timespec timespec;
  struct timespec *ts = NULL;

  if (!wb) {
    return -1;
  }

  if (!axis_atomic_load(&g_futex_supported)) {
    return __busy_loop(&wb->sig, expect, lock, timeout);
  }

  if (timeout == 0) {
    // only a test
    return (__sync_add_and_fetch(&wb->sig, 0) != expect) ? 0 : -1;
  }

  int64_t timeout_time = axis_current_time() + timeout;

  if (timeout > 0) {
    ts = &timespec;
  } else {
    ts = NULL;
  }

  while (__sync_add_and_fetch(&wb->sig, 0) == expect) {
    if (ts) {
      int64_t diff = timeout_time - axis_current_time();
      if (diff < 0) {
        return -1;
      }

      ts->tv_sec = diff / 1000;
      ts->tv_nsec = (diff % 1000) * 1000000;
    }

    axis_spinlock_unlock(lock);
    int ret = __futex((int *)&wb->sig, FUTEX_WAIT_OP, (int)expect, ts);
    axis_spinlock_lock(lock);
    if (ret < 0) {
      if (errno == EAGAIN) {
        continue;
      }

      return ret;
    }
  }

  return 0;
}

void axis_waitable_notify(axis_waitable_t *wb) {
  if (!wb) {
    return;
  }

  if (!axis_atomic_load(&g_futex_supported)) {
    return;
  }

  __futex((int *)&wb->sig, FUTEX_WAKE_OP, 1, NULL);
}

void axis_waitable_notify_all(axis_waitable_t *wb) {
  if (!wb) {
    return;
  }

  if (!axis_atomic_load(&g_futex_supported)) {
    return;
  }

  __futex((int *)&wb->sig, FUTEX_WAKE_OP, INT32_MAX, NULL);
}
