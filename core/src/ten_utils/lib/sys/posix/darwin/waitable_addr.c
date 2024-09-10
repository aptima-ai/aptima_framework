//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include "ten_utils/lib/waitable_addr.h"

#include <memory.h>

TEN_UTILS_PRIVATE_API int __busy_loop(volatile uint32_t *addr, uint32_t expect,
                                      ten_spinlock_t *lock, int timeout);

void ten_waitable_init(ten_waitable_t *wb) {
  static const ten_waitable_t initializer = TEN_WAITABLE_INIT;
  *wb = initializer;
}

ten_waitable_t *ten_waitable_from_addr(uint32_t *address) {
  ten_waitable_t *ret = (ten_waitable_t *)address;

  if (!address) {
    return NULL;
  }

  ten_waitable_init(ret);
  return ret;
}

int ten_waitable_wait(ten_waitable_t *wb, uint32_t expect, ten_spinlock_t *lock,
                      int timeout) {
  return __busy_loop(&wb->sig, expect, lock, timeout);
}

void ten_waitable_notify(ten_waitable_t *wb) {
  // Nothing we can do
  // Also not possible to notify single waiter
}

void ten_waitable_notify_all(ten_waitable_t *wb) {
  // Nothing we can do
}
