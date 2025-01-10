//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_utils/io/general/transport/backend/base.h"

#include <stdlib.h>
#include <string.h>

#include "include_internal/axis_utils/io/runloop.h"
#include "axis_utils/io/stream.h"
#include "axis_utils/io/transport.h"

void axis_transportbackend_init(axis_transportbackend_t *self,
                               axis_transport_t *transport,
                               const axis_string_t *name) {
  assert(self);

  axis_atomic_store(&self->is_close, false);
  self->transport = transport;
  self->name = axis_string_create_formatted("%.*s", name->buf_size, name->buf);
  self->impl = strdup(transport->loop->impl);
}

void axis_transportbackend_deinit(axis_transportbackend_t *self) {
  assert(self);
  if (self->name) {
    axis_string_destroy(self->name);
  }

  if (self->impl) {
    free((void *)self->impl);
  }
}

void axis_streambackend_init(const char *impl, axis_streambackend_t *backend,
                            axis_stream_t *stream) {
  assert(backend && stream);

  axis_atomic_store(&backend->is_close, false);
  backend->stream = stream;

  stream->backend = backend;
  backend->impl = strdup(impl);
}

void axis_streambackend_deinit(axis_streambackend_t *backend) {
  assert(backend);
  if (backend->impl) {
    free((void *)backend->impl);
  }
}
