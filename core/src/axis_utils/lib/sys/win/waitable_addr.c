//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_utils/lib/waitable_addr.h"

#include <Windows.h>

#include "axis_utils/lib/atomic.h"
#include "axis_utils/lib/spinlock.h"
#include "axis_utils/lib/thread_once.h"
#include "axis_utils/lib/time.h"

axis_UTILS_PRIVATE_API int __busy_loop(volatile uint32_t *addr, uint32_t expect,
                                      axis_spinlock_t *lock, int timeout);

typedef BOOL(__stdcall *FpWaitOnAddress)(volatile VOID *Address,
                                         PVOID CompareAddress,
                                         SIZE_T AddressSize,
                                         DWORD dwMilliseconds);
typedef void(__stdcall *FpWakeByAddressSingle)(PVOID Address);
typedef void(__stdcall *FpWakeByAddressAll)(PVOID Address);

static FpWaitOnAddress fp_wait_on_address = NULL;
static FpWakeByAddressSingle fp_wake_by_address_single = NULL;
static FpWakeByAddressAll fp_wake_by_address_all = NULL;
static axis_thread_once_t g_detect = axis_THREAD_ONCE_INIT;
static axis_atomic_t g_system_supported = 0;

static void __detect_system_callback(void) {
  HANDLE handle = LoadLibraryA("API-MS-Win-Core-Synch-l1-2-0.dll");
  if (handle == NULL) {
    return;
  }

  fp_wait_on_address = (FpWaitOnAddress)GetProcAddress(handle, "WaitOnAddress");
  fp_wake_by_address_single =
      (FpWakeByAddressSingle)GetProcAddress(handle, "WakeByAddressSingle");
  fp_wake_by_address_all =
      (FpWakeByAddressSingle)GetProcAddress(handle, "WakeByAddressAll");
  if (!fp_wait_on_address || !fp_wake_by_address_single ||
      !fp_wake_by_address_all) {
    FreeLibrary(handle);
    handle = NULL;
    return;
  }

  axis_atomic_store(&g_system_supported, 1);
}

void axis_waitable_init(axis_waitable_t *wb) {
  static const axis_waitable_t initializer = axis_WAITABLE_INIT;

  axis_thread_once(&g_detect, __detect_system_callback);

  *wb = initializer;
}

axis_waitable_t *axis_waitable_from_addr(uint32_t *address) {
  axis_waitable_t *waitable = (axis_waitable_t *)address;

  if (!address) {
    return NULL;
  }

  axis_waitable_init(waitable);
  return waitable;
}

int axis_waitable_wait(axis_waitable_t *wb, uint32_t expect, axis_spinlock_t *lock,
                      int timeout) {
  if (!wb) {
    return -1;
  }

  if (!axis_atomic_load(&g_system_supported)) {
    return __busy_loop(&wb->sig, expect, lock, timeout);
  }

  if (timeout == 0) {
    // only a test
    return (InterlockedAdd(&wb->sig, 0) != expect) ? 0 : -1;
  }

  int64_t timeout_time = (timeout < 0) ? -1 : (axis_current_time() + timeout);

  while (InterlockedAdd(&wb->sig, 0) == expect) {
    int64_t diff = INFINITE;

    if (timeout > 0) {
      int64_t diff = timeout_time - axis_current_time();
      if (diff < 0) {
        return -1;
      }
    }

    axis_spinlock_unlock(lock);
    BOOL ret = fp_wait_on_address(&wb->sig, &expect, sizeof(int32_t), diff);
    axis_spinlock_lock(lock);
    if (!ret) {
      return -1;
    }
  }

  return 0;
}

void axis_waitable_notify(axis_waitable_t *wb) {
  if (!wb) {
    return;
  }

  if (!axis_atomic_load(&g_system_supported)) {
    return;
  }

  fp_wake_by_address_single(&wb->sig);
}

void axis_waitable_notify_all(axis_waitable_t *wb) {
  if (!wb) {
    return;
  }

  if (!axis_atomic_load(&g_system_supported)) {
    return;
  }

  fp_wake_by_address_all(&wb->sig);
}
