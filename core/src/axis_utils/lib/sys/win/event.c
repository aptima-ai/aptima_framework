//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_utils/lib/event.h"

#include <Windows.h>
#include <stdlib.h>

typedef struct axis_event_t {
  HANDLE event;
} axis_event_t;

axis_event_t *axis_event_create(int init_state, int auto_reset) {
  axis_event_t *event = (axis_event_t *)malloc(sizeof(*event));

  if (!event) {
    return NULL;
  }

  event->event = CreateEvent(NULL, !auto_reset, init_state, NULL);
  return event;
}

int axis_event_wait(axis_event_t *event, int wait_ms) {
  if (!event || !event->event) {
    return -1;
  }

  return (WaitForSingleObject(event->event, wait_ms) == WAIT_OBJECT_0) ? 0 : -1;
}

void axis_event_set(axis_event_t *event) {
  if (!event || !event->event) {
    return;
  }

  SetEvent(event->event);
}

void axis_event_reset(axis_event_t *event) {
  if (!event || !event->event) {
    return;
  }

  ResetEvent(event->event);
}

void axis_event_destroy(axis_event_t *event) {
  if (!event) {
    return;
  }

  if (event->event) {
    CloseHandle(event->event);
  }

  free(event);
}
