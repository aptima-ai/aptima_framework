//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_utils/lib/mutex.h"

#include <inttypes.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>

#include "include_internal/axis_utils/lib/mutex.h"
#include "axis_utils/lib/alloc.h"
#include "axis_utils/lib/signature.h"
#include "axis_utils/lib/thread.h"

typedef struct axis_mutex_t {
  axis_signature_t signature;
  pthread_mutex_t mutex;
  axis_tid_t owner;
} axis_mutex_t;

static bool axis_mutex_check_integrity(axis_mutex_t *self) {
  axis_ASSERT(self, "Invalid argument.");

  if (axis_signature_get(&self->signature) != axis_MUTEX_SIGNATURE) {
    return false;
  }

  return true;
}

axis_mutex_t *axis_mutex_create(void) {
  axis_mutex_t *mutex = axis_malloc(sizeof(*mutex));
  axis_ASSERT(mutex, "Failed to allocate memory.");
  if (mutex == NULL) {
    return NULL;
  }

  axis_signature_set(&mutex->signature, axis_MUTEX_SIGNATURE);

#if defined(_DEBUG)
  // Enable more debugging mechanisms in the debug build.
  pthread_mutexattr_t attr;
  pthread_mutexattr_init(&attr);
  pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
#endif

  pthread_mutex_init(&mutex->mutex,
#if defined(_DEBUG)
                     &attr
#else
                     NULL
#endif
  );

#if defined(_DEBUG)
  pthread_mutexattr_destroy(&attr);
#endif

  return mutex;
}

int axis_mutex_lock(axis_mutex_t *mutex) {
  axis_ASSERT(mutex, "Invalid argument.");
  if (!mutex) {
    return -1;
  }

  axis_ASSERT(axis_mutex_check_integrity(mutex), "Invalid argument.");

  int rc = pthread_mutex_lock(&mutex->mutex);
  if (rc) {
    axis_ASSERT(0, "Should not happen: %d", rc);
  }

  mutex->owner = axis_thread_get_id(NULL);
  axis_ASSERT(mutex->owner, "Should not happen.");

  return rc;
}

int axis_mutex_unlock(axis_mutex_t *mutex) {
  axis_ASSERT(mutex && axis_mutex_check_integrity(mutex), "Invalid argument.");
  if (!mutex) {
    return -1;
  }

  axis_ASSERT(mutex->owner, "Should not happen.");
  axis_tid_t prev_owner = mutex->owner;
  mutex->owner = 0;

  int rc = pthread_mutex_unlock(&mutex->mutex);
  axis_ASSERT(!rc,
             "Should not happen: %d. unlock by: %" PRId64
             ", but hold by: %" PRId64,
             rc, axis_thread_get_id(NULL), prev_owner);

  return rc;
}

void axis_mutex_destroy(axis_mutex_t *mutex) {
  axis_ASSERT(mutex && axis_mutex_check_integrity(mutex), "Invalid argument.");
  if (!mutex) {
    return;
  }

  pthread_mutex_destroy(&mutex->mutex);
  axis_signature_set(&mutex->signature, 0);
  mutex->owner = 0;

  axis_free(mutex);
}

void *axis_mutex_get_native_handle(axis_mutex_t *mutex) {
  axis_ASSERT(mutex && axis_mutex_check_integrity(mutex), "Invalid argument.");
  if (!mutex) {
    return NULL;
  }

  return &mutex->mutex;
}

void axis_mutex_set_owner(axis_mutex_t *mutex, axis_tid_t owner) {
  axis_ASSERT(mutex && owner, "Invalid argument.");
  if (!mutex) {
    return;
  }

  mutex->owner = owner;
}
