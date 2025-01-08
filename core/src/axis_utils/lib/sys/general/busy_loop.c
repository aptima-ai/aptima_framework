//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include <stdint.h>

#include "axis_utils/lib/spinlock.h"
#include "axis_utils/lib/thread.h"
#include "axis_utils/lib/time.h"
#include "axis_utils/macro/mark.h"

#if defined(_WIN32)
#include <Windows.h>
#define ATOMIC_LOAD32(a) InterlockedAdd((a), 0)
#else
#define ATOMIC_LOAD32(a) __sync_add_and_fetch((a), 0)
#endif

int __busy_loop(volatile uint32_t *addr, uint32_t expect, axis_spinlock_t *lock,
                int timeout) {
  int64_t timeout_time = 0;
  uint64_t loops = 0;
  int ret = 0;

  if (!addr) {
    return -1;
  }

  if (!timeout) {
    return (ATOMIC_LOAD32(addr) != expect) ? 0 : -1;
  }

  timeout_time = timeout < 0 ? -1 : axis_current_time() + timeout;
  loops = 0;

  axis_spinlock_unlock(lock);
  // unsafe test first so it can fail quickly in content
  // heavy envrionment
  while (*addr == expect) {
    // then safe test
    if (UNLIKELY(ATOMIC_LOAD32(addr) != expect)) {
      continue;
    }

    int64_t diff = -1;

    if (timeout > 0) {
      diff = timeout_time - axis_current_time();
      if (timeout_time > 0 && diff < 0) {
        ret = -1;
        break;
      }
    }

    loops++;
    if (loops < 50) {
      continue;
    }

    // relax
    if (loops < 200) {
      axis_thread_pause_cpu();
      continue;
    }

    // relax even more
    if (loops < 500) {
      axis_thread_yield();
      continue;
    }

    // relax deeply
    axis_sleep(20);
  }

  axis_spinlock_lock(lock);
  return ret;
}
