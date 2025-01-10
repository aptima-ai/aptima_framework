//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_utils/io/transport.h"

#include <stdlib.h>

#include "include_internal/axis_utils/io/runloop.h"
#include "axis_utils/io/general/transport/backend/base.h"
#include "axis_utils/io/general/transport/backend/factory.h"
#include "axis_utils/lib/mutex.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/macro/mark.h"

// Destroy all the resources hold by this transport object.
static void axis_transport_destroy(axis_transport_t *self) {
  axis_ASSERT(self, "Invalid argument.");

  if (self->lock) {
    axis_mutex_destroy(self->lock);
  }
  free(self);
}

axis_transport_t *axis_transport_create(axis_runloop_t *loop) {
  axis_transport_t *self = NULL;

  if (!loop) {
    goto error;
  }

  self = (axis_transport_t *)malloc(sizeof(*self));
  if (!self) {
    goto error;
  }

  memset(self, 0, sizeof(*self));

  axis_atomic_store(&self->close, 0);

  self->lock = axis_mutex_create();
  if (!self->lock) {
    goto error;
  }

  self->loop = loop;
  self->user_data = NULL;
  self->backend = NULL;
  self->on_server_connected = NULL;
  self->on_server_connected_data = NULL;
  self->on_client_accepted = NULL;
  self->on_client_accepted_data = NULL;
  self->on_closed = NULL;
  self->on_closed_data = NULL;
  self->drop_type = axis_TRANSPORT_DROP_NEW;
  self->drop_when_full = 1;

  return self;

error:
  if (self) {
    axis_transport_destroy(self);
  }
  return NULL;
}

void axis_transport_set_close_cb(axis_transport_t *self, void *close_cb,
                                void *close_cb_data) {
  axis_ASSERT(self, "Invalid argument.");

  self->on_closed = close_cb;
  self->on_closed_data = close_cb_data;
}

// The actual closing flow.
void axis_transport_on_close(axis_transport_t *self) {
  axis_ASSERT(self, "Invalid argument.");

  // The final step in the closing flow is to notify the outer environment that
  // we are closed.
  if (self->on_closed) {
    self->on_closed(self->on_closed_data);
  }

  axis_transport_destroy(self);
}

int axis_transport_close(axis_transport_t *self) {
  axis_ASSERT(self, "Invalid argument.");

  if (axis_atomic_bool_compare_swap(&self->close, 0, 1)) {
    // Trigger the closing flow of the backend, or proceed the closing flow
    // directly.
    if (self->backend) {
      self->backend->close(self->backend);
    } else {
      axis_transport_on_close(self);
    }

    return 0;
  }

  return -1;
}

enum axis_TRANSPORT_DROP_TYPE axis_transport_get_drop_type(
    axis_transport_t *self) {
  axis_TRANSPORT_DROP_TYPE ret = axis_TRANSPORT_DROP_NEW;

  if (!self) {
    return ret;
  }

  axis_UNUSED int rc = axis_mutex_lock(self->lock);
  axis_ASSERT(!rc, "Invalid argument.");

  ret = self->drop_type;

  rc = axis_mutex_unlock(self->lock);
  axis_ASSERT(!rc, "Invalid argument.");

  return ret;
}

void axis_transport_set_drop_type(axis_transport_t *self,
                                 axis_TRANSPORT_DROP_TYPE drop_type) {
  if (!self) {
    return;
  }

  axis_UNUSED int rc = axis_mutex_lock(self->lock);
  axis_ASSERT(!rc, "Invalid argument.");

  self->drop_type = drop_type;

  rc = axis_mutex_unlock(self->lock);
  axis_ASSERT(!rc, "Invalid argument.");
}

int axis_transport_drop_required(axis_transport_t *self) {
  int ret = 0;

  if (!self) {
    return ret;
  }

  axis_UNUSED int rc = axis_mutex_lock(self->lock);
  axis_ASSERT(!rc, "Invalid argument.");

  ret = self->drop_when_full;

  rc = axis_mutex_unlock(self->lock);
  axis_ASSERT(!rc, "Invalid argument.");

  return ret;
}

void axis_transport_set_drop_when_full(axis_transport_t *self, int drop) {
  if (!self) {
    return;
  }

  axis_UNUSED int rc = axis_mutex_lock(self->lock);
  axis_ASSERT(!rc, "Invalid argument.");

  self->drop_when_full = drop;

  rc = axis_mutex_unlock(self->lock);
  axis_ASSERT(!rc, "Invalid argument.");
}

int axis_transport_listen(axis_transport_t *self, const axis_string_t *my_uri) {
  const axis_transportbackend_factory_t *factory = NULL;

  if (!self || self->backend) {
    // axis_LOGE("Empty transport");
    return -1;
  }

  if (!my_uri || axis_string_is_empty(my_uri)) {
    // axis_LOGD("Empty uri");
    return -1;
  }

  factory = axis_get_transportbackend_factory(self->loop->impl, my_uri);
  if (!factory) {
    // axis_LOGE("No valid backend for uri %s", my_uri->buf);
    return -1;
  }

  self->backend = factory->create(self, my_uri);
  if (!self->backend) {
    // axis_LOGE("No valid backend for uri %s", my_uri->buf);
    return -1;
  }

  return self->backend->listen(self->backend, my_uri);
}

int axis_transport_connect(axis_transport_t *self, axis_string_t *dest) {
  axis_ASSERT(self && dest, "Invalid argument.");

  if (!self || !dest) {
    return -1;
  }

  const axis_transportbackend_factory_t *factory =
      axis_get_transportbackend_factory(self->loop->impl, dest);
  if (!factory) {
    return -1;
  }

  axis_transportbackend_t *backend = factory->create(self, dest);
  if (!backend) {
    return -1;
  }

  self->backend = backend;
  return backend->connect(backend, dest);
}
