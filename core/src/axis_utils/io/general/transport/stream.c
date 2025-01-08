//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_utils/io/stream.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "axis_utils/io/general/transport/backend/base.h"
#include "axis_utils/lib/signature.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/macro/memory.h"

bool axis_stream_check_integrity(axis_stream_t *self) {
  axis_ASSERT(self, "Invalid argument.");

  if (axis_signature_get(&self->signature) != axis_STREAM_SIGNATURE) {
    return false;
  }
  return true;
}

void axis_stream_init(axis_stream_t *self) {
  axis_ASSERT(self, "Invalid argument.");

  axis_signature_set(&self->signature, axis_STREAM_SIGNATURE);
  axis_atomic_store(&self->close, 0);
  self->on_message_free = NULL;
  self->on_message_read = NULL;
  self->on_message_sent = NULL;
  self->on_closed = NULL;
  self->on_closed_data = NULL;
}

int axis_stream_send(axis_stream_t *self, const char *msg, uint32_t size,
                    void *user_data) {
  axis_ASSERT(self && axis_stream_check_integrity(self) && msg && (size > 0) &&
                 self->backend,
             "Invalid argument.");

  return self->backend->write(self->backend, msg, size, user_data);
}

int axis_stream_start_read(axis_stream_t *self) {
  axis_ASSERT(self && axis_stream_check_integrity(self) && self->backend,
             "Invalid argument.");

  return self->backend->start_read(self->backend);
}

int axis_stream_stop_read(axis_stream_t *self) {
  axis_ASSERT(self && axis_stream_check_integrity(self) && self->backend,
             "Invalid argument.");

  return self->backend->stop_read(self->backend);
}

static void axis_stream_destroy(axis_stream_t *self) {
  axis_ASSERT(self && axis_stream_check_integrity(self), "Invalid argument.");

  axis_FREE(self);
}

void axis_stream_on_close(axis_stream_t *self) {
  axis_ASSERT(self && axis_stream_check_integrity(self), "Invalid argument.");

  if (self->on_closed) {
    self->on_closed(self->on_closed_data);
  }
  axis_stream_destroy(self);
}

void axis_stream_close(axis_stream_t *self) {
  axis_ASSERT(self && axis_stream_check_integrity(self), "Invalid argument.");
  axis_ASSERT(self->backend, "Invalid argument.");

  if (axis_atomic_bool_compare_swap(&self->close, 0, 1)) {
    self->backend->close(self->backend);
  }
}

void axis_stream_set_on_closed(axis_stream_t *self, void *on_closed,
                              void *on_closed_data) {
  axis_ASSERT(self && axis_stream_check_integrity(self), "Invalid argument.");

  self->on_closed = on_closed;
  self->on_closed_data = on_closed_data;
}
