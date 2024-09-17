//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "ten_utils/lib/cond.h"

#include <Windows.h>
#include <stdlib.h>

#include "ten_utils/macro/check.h"
#include "ten_utils/lib/alloc.h"
#include "ten_utils/lib/mutex.h"
#include "ten_utils/lib/time.h"

struct ten_cond_t {
  CONDITION_VARIABLE cond;
};

ten_cond_t *ten_cond_create(void) {
  ten_cond_t *cond = (ten_cond_t *)TEN_MALLOC(sizeof(ten_cond_t));
  TEN_ASSERT(cond, "Failed to allocate memory.");
  if (cond == NULL) {
    return NULL;
  }

  InitializeConditionVariable(&cond->cond);
  return cond;
}

void ten_cond_destroy(ten_cond_t *cond) {
  if (!cond) {
    TEN_LOGE("Invalid_argument.");
    return;
  }

  free(cond);
}

int ten_cond_wait(ten_cond_t *cond, ten_mutex_t *mutex, int64_t wait_ms) {
  CRITICAL_SECTION *lock =
      (CRITICAL_SECTION *)ten_mutex_get_native_handle(mutex);

  if (!cond || !lock) {
    return -1;
  }

  return SleepConditionVariableCS(&cond->cond, lock, wait_ms) ? 0 : -1;
}

int ten_cond_wait_while(ten_cond_t *cond, ten_mutex_t *mutex,
                        int (*predicate)(void *), void *arg, int64_t wait_ms) {
  BOOL ret = FALSE;
  BOOL wait_forever;
  CRITICAL_SECTION *lock =
      (CRITICAL_SECTION *)ten_mutex_get_native_handle(mutex);

  if (!cond || !mutex || !predicate || !lock) {
    TEN_LOGE("Invalid_argument.");
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
      int64_t begin = ten_current_time();
      ret = SleepConditionVariableCS(&cond->cond, lock, wait_ms);
      wait_ms -= (ten_current_time() - begin);
    }

    if (!ret) {
      return -1;
    }
  }

  return 0;
}

int ten_cond_signal(ten_cond_t *cond) {
  if (!cond) {
    TEN_LOGE("Invalid_argument.");
    return -1;
  }

  WakeConditionVariable(&cond->cond);
  return 0;
}

int ten_cond_broadcast(ten_cond_t *cond) {
  if (!cond) {
    TEN_LOGE("Invalid_argument.");
    return -1;
  }

  WakeAllConditionVariable(&cond->cond);
  return 0;
}
