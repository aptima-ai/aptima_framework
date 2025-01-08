//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_utils/lib/mutex.h"

#include <Windows.h>
#include <stdlib.h>

#include "axis_utils/lib/alloc.h"

/**
 * Be careful that mutex from `CreateMutex` is sadly slow
 * Actually the counterpart of pthread_mutex _is_ CRITICAL_SECTION by any means
 */
typedef struct axis_mutex_t {
  CRITICAL_SECTION section;
} axis_mutex_t;

axis_mutex_t *axis_mutex_create(void) {
  axis_mutex_t *mutex = (axis_mutex_t *)axis_MALLOC(sizeof(*mutex));
  axis_ASSERT(mutex, "Failed to allocate memory.");
  if (!mutex) {
    return NULL;
  }

  InitializeCriticalSection(&mutex->section);
  return mutex;
}

int axis_mutex_lock(axis_mutex_t *mutex) {
  axis_ASSERT(mutex, "Invalid argument.");
  if (!mutex) {
    return -1;
  }

  EnterCriticalSection(&mutex->section);
  return 0;
}

int axis_mutex_unlock(axis_mutex_t *mutex) {
  axis_ASSERT(mutex, "Invalid argument.");
  if (!mutex) {
    return -1;
  }

  LeaveCriticalSection(&mutex->section);
  return 0;
}

void axis_mutex_destroy(axis_mutex_t *mutex) {
  axis_ASSERT(mutex, "Invalid argument.");
  if (!mutex) {
    return;
  }

  DeleteCriticalSection(&mutex->section);
  axis_FREE(mutex);
}

void *axis_mutex_get_native_handle(axis_mutex_t *mutex) {
  axis_ASSERT(mutex, "Invalid argument.");
  if (!mutex) {
    return NULL;
  }

  return &mutex->section;
}
