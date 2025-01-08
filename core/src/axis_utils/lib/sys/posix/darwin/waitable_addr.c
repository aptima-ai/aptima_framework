//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_utils/lib/waitable_addr.h"

#include <memory.h>

axis_UTILS_PRIVATE_API int __busy_loop(volatile uint32_t *addr, uint32_t expect,
                                      axis_spinlock_t *lock, int timeout);

void axis_waitable_init(axis_waitable_t *wb) {
  static const axis_waitable_t initializer = axis_WAITABLE_INIT;
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
  return __busy_loop(&wb->sig, expect, lock, timeout);
}

void axis_waitable_notify(axis_waitable_t *wb) {
  // Nothing we can do
  // Also not possible to notify single waiter
}

void axis_waitable_notify_all(axis_waitable_t *wb) {
  // Nothing we can do
}
