//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_utils/lib/cond.h"

#include <Windows.h>
#include <stdlib.h>

#include "axis_utils/lib/alloc.h"
#include "axis_utils/lib/mutex.h"
#include "axis_utils/lib/time.h"
#include "axis_utils/log/log.h"
#include "axis_utils/macro/check.h"

struct axis_cond_t {
  CONDITION_VARIABLE cond;
};

axis_cond_t *axis_cond_create(void) {
  axis_cond_t *cond = (axis_cond_t *)axis_MALLOC(sizeof(axis_cond_t));
  axis_ASSERT(cond, "Failed to allocate memory.");
  if (cond == NULL) {
    return NULL;
  }

  InitializeConditionVariable(&cond->cond);
  return cond;
}

void axis_cond_destroy(axis_cond_t *cond) {
  if (!cond) {
    axis_LOGE("Invalid_argument.");
    return;
  }

  free(cond);
}

int axis_cond_wait(axis_cond_t *cond, axis_mutex_t *mutex, int64_t wait_ms) {
  CRITICAL_SECTION *lock =
      (CRITICAL_SECTION *)axis_mutex_get_native_handle(mutex);

  if (!cond || !lock) {
    return -1;
  }

  return SleepConditionVariableCS(&cond->cond, lock, wait_ms) ? 0 : -1;
}

int axis_cond_wait_while(axis_cond_t *cond, axis_mutex_t *mutex,
                        int (*predicate)(void *), void *arg, int64_t wait_ms) {
  BOOL ret = FALSE;
  BOOL wait_forever;
  CRITICAL_SECTION *lock =
      (CRITICAL_SECTION *)axis_mutex_get_native_handle(mutex);

  if (!cond || !mutex || !predicate || !lock) {
    axis_LOGE("Invalid_argument.");
    return -1;
  }

  if (wait_ms == 0) {
    int test_result = predicate(arg);

    return test_result ? -1 : 0;
  }

  wait_forever = wait_ms < 0;

  while (predicate(arg)) {
    if (wait_forever) {
      ret = SleepConditionVariableCS(&cond->cond, lock, -1);
    } else if (wait_ms < 0) {
      return -1;
    } else {
      int64_t begin = axis_current_time();
      ret = SleepConditionVariableCS(&cond->cond, lock, wait_ms);
      wait_ms -= (axis_current_time() - begin);
    }

    if (!ret) {
      return -1;
    }
  }

  return 0;
}

int axis_cond_signal(axis_cond_t *cond) {
  if (!cond) {
    axis_LOGE("Invalid_argument.");
    return -1;
  }

  WakeConditionVariable(&cond->cond);
  return 0;
}

int axis_cond_broadcast(axis_cond_t *cond) {
  if (!cond) {
    axis_LOGE("Invalid_argument.");
    return -1;
  }

  WakeAllConditionVariable(&cond->cond);
  return 0;
}
