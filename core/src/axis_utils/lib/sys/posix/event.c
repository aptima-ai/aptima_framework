//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_utils/lib/event.h"

#include <stdlib.h>
#include <sys/time.h>

#include "include_internal/axis_utils/lib/mutex.h"
#include "axis_utils/lib/alloc.h"
#include "axis_utils/lib/cond.h"
#include "axis_utils/lib/mutex.h"
#include "axis_utils/lib/signature.h"
#include "axis_utils/lib/thread.h"
#include "axis_utils/macro/check.h"

#define axis_EVENT_SIGNATURE 0xB5F7D324A07B41E4U

typedef struct axis_event_t {
  axis_signature_t signature;

  axis_mutex_t *mutex;
  axis_cond_t *cond;
  int signal;
  int auto_reset;
} axis_event_t;

static int axis_event_check_integrity(axis_event_t *self) {
  axis_ASSERT(self, "Should not happen.");

  if (axis_signature_get(&self->signature) != axis_EVENT_SIGNATURE) {
    return 0;
  }
  return 1;
}

static void axis_event_init(axis_event_t *event, int init_state, int auto_reset) {
  axis_ASSERT(event, "Invalid argument.");

  axis_signature_set(&event->signature, axis_EVENT_SIGNATURE);

  event->mutex = axis_mutex_create();
  event->cond = axis_cond_create();
  event->signal = init_state;
  event->auto_reset = auto_reset;
}

axis_event_t *axis_event_create(int init_state, int auto_reset) {
  axis_event_t *event = axis_MALLOC(sizeof(*event));
  axis_ASSERT(event, "Failed to allocate memory.");

  axis_event_init(event, init_state, auto_reset);

  return event;
}

static int axis_event_no_signal(void *arg) {
  axis_event_t *event = (axis_event_t *)arg;
  axis_ASSERT(event && axis_event_check_integrity(event), "Invalid argument.");

  return !event->signal;
}

int axis_event_wait(axis_event_t *event, int wait_ms) {
  axis_ASSERT(event && axis_event_check_integrity(event), "Invalid argument.");

  int ret = -1;
  struct timespec ts;
  struct timeval tv;
  struct timespec *abs_time = NULL;

  axis_mutex_lock(event->mutex);

  ret = axis_cond_wait_while(event->cond, event->mutex, axis_event_no_signal,
                            event, wait_ms);

  axis_mutex_set_owner(event->mutex, axis_thread_get_id(NULL));

  if (event->auto_reset) {
    event->signal = 0;
  }

  axis_mutex_unlock(event->mutex);

  return ret;
}

void axis_event_set(axis_event_t *event) {
  axis_ASSERT(event && axis_event_check_integrity(event), "Invalid argument.");

  axis_mutex_lock(event->mutex);
  event->signal = 1;
  if (event->auto_reset) {
    axis_cond_signal(event->cond);
  } else {
    axis_cond_broadcast(event->cond);
  }
  axis_mutex_unlock(event->mutex);
}

void axis_event_reset(axis_event_t *event) {
  axis_ASSERT(event && axis_event_check_integrity(event), "Invalid argument.");

  if (event->auto_reset) {
    return;
  }

  axis_mutex_lock(event->mutex);
  event->signal = 0;
  axis_mutex_unlock(event->mutex);
}

static void axis_event_deinit(axis_event_t *event) {
  axis_ASSERT(event && axis_event_check_integrity(event), "Invalid argument.");

  axis_mutex_destroy(event->mutex);
  axis_cond_destroy(event->cond);
}

void axis_event_destroy(axis_event_t *event) {
  axis_ASSERT(event && axis_event_check_integrity(event), "Invalid argument.");

  axis_event_deinit(event);
  axis_FREE(event);
}
