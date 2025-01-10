//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_utils/lib/cond.h"

#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/time.h>

#include "axis_utils/lib/alloc.h"
#include "axis_utils/lib/mutex.h"
#include "axis_utils/log/log.h"
#include "axis_utils/macro/memory.h"

struct axis_cond_t {
  pthread_cond_t cond;
};

axis_cond_t *axis_cond_create(void) {
  axis_cond_t *cond = (axis_cond_t *)axis_MALLOC(sizeof(axis_cond_t));
  if (!cond) {
    axis_LOGE("Failed to allocate memory.");
    return NULL;
  }

  pthread_cond_init(&cond->cond, NULL);
  return cond;
}

void axis_cond_destroy(axis_cond_t *cond) {
  if (!cond) {
    axis_LOGE("Invalid argument.");
    return;
  }

  pthread_cond_destroy(&cond->cond);
  axis_FREE(cond);
}

int axis_cond_wait(axis_cond_t *cond, axis_mutex_t *mutex, int64_t wait_ms) {
  struct timeval tv;
  struct timespec ts;
  pthread_mutex_t *lock = (pthread_mutex_t *)axis_mutex_get_native_handle(mutex);
  if (!cond || !lock) {
    axis_LOGE("Invalid_argument.");
    return -1;
  }

  if (wait_ms < 0) {
    return pthread_cond_wait(&cond->cond, lock);
  } else {
    gettimeofday(&tv, NULL);
    ts.tv_sec = tv.tv_sec + wait_ms / 1000;
    ts.tv_nsec = (tv.tv_usec + (wait_ms % 1000) * 1000) * 1000;
    return pthread_cond_timedwait(&cond->cond, lock, &ts);
  }
}

int axis_cond_wait_while(axis_cond_t *cond, axis_mutex_t *mutex,
                        int (*predicate)(void *), void *arg, int64_t wait_ms) {
  int ret = -1;
  struct timespec ts;
  struct timeval tv;
  struct timespec *abs_time = NULL;
  pthread_mutex_t *lock = (pthread_mutex_t *)axis_mutex_get_native_handle(mutex);

  if (!cond || !mutex || !predicate || !lock) {
    axis_LOGE("Invalid_argument.");
    return -1;
  }

  if (wait_ms == 0) {
    int test_result = predicate(arg);
    return test_result ? -1 : 0;
  }

  if (wait_ms > 0) {
    gettimeofday(&tv, NULL);
    ts.tv_sec = tv.tv_sec + wait_ms / 1000;
    ts.tv_nsec = (tv.tv_usec + (wait_ms % 1000) * 1000) * 1000;
    abs_time = &ts;
  }

  while (predicate(arg)) {
    if (abs_time) {
      ret = pthread_cond_timedwait(&cond->cond, lock, abs_time);
    } else {
      ret = pthread_cond_wait(&cond->cond, lock);
    }

    if (ret == ETIMEDOUT || ret == EINVAL) {
      return -ret;
    }
  }

  return 0;
}

int axis_cond_signal(axis_cond_t *cond) {
  if (!cond) {
    axis_LOGE("Invalid_argument.");
    return -1;
  }

  return pthread_cond_signal(&cond->cond);
}

int axis_cond_broadcast(axis_cond_t *cond) {
  if (!cond) {
    axis_LOGE("Invalid_argument.");
    return -1;
  }

  return pthread_cond_broadcast(&cond->cond);
}
