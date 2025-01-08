//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_utils/io/general/transport/backend/uv/stream/pipe.h"

#include <stdlib.h>
#include <uv.h>

#include "include_internal/axis_utils/io/transport.h"
#include "axis_utils/io/general/transport/backend/base.h"
#include "axis_utils/io/stream.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/lib/uri.h"

typedef struct axis_transportbackend_pipe_t {
  axis_transportbackend_t base;

  uv_pipe_t *server;
} axis_transportbackend_pipe_t;

static axis_string_t *__get_pipe_name(const axis_string_t *uri) {
  if (uri == NULL) {
    return NULL;
  }

  axis_string_t *host = axis_uri_get_host(axis_string_get_raw_str(uri));
  if (host == NULL) {
    return NULL;
  }

#if defined(_WIN32)

  if (host->buf_size > 3 && host->buf[0] == '\\' && host->buf[1] == '\\' &&
      host->buf[2] == '?' && host->buf[3] == '\\') {
    return host;
  }

  axis_string_t *ret =
      axis_string_create_formatted("\\\\?\\pipe\\%s.sock", host->buf);
  axis_string_destroy(host);
  return ret;

#else

  if (host->buf_size > 0 && host->buf[0] == '/') {
    return host;
  }

  axis_string_t *ret = axis_string_create_formatted("/tmp/%s.sock", host->buf);
  axis_string_destroy(host);
  return ret;

#endif
}

// Destroy all the resources hold by tpbackend object. Call this only when all
// the closing flow is completed.
static void axis_transportbackend_pipe_destroy(
    axis_transportbackend_pipe_t *self) {
  if (!self) {
    return;
  }

  axis_transportbackend_deinit(&self->base);
  free(self);
}

static void axis_transportbackend_pipe_on_close(
    axis_transportbackend_pipe_t *self) {
  assert(self);

  axis_transport_t *transport = self->base.transport;
  assert(transport);
  axis_transport_on_close(transport);

  axis_transportbackend_pipe_destroy(self);
}

static void on_pipe_server_closed(uv_handle_t *handle) {
  assert(handle && handle->data);
  axis_transportbackend_pipe_t *self =
      (axis_transportbackend_pipe_t *)handle->data;
  free(handle);

  // Proceed the closing flow.
  axis_transportbackend_pipe_on_close(self);
}

// Trigger the closing flow.
static void axis_transportbackend_pipe_close(axis_transportbackend_t *backend) {
  axis_transportbackend_pipe_t *self = (axis_transportbackend_pipe_t *)backend;

  if (!self) {
    // axis_LOGD("empty handle, treat as fail");
    return;
  }

  if (axis_atomic_bool_compare_swap(&self->base.is_close, 0, 1)) {
    // axis_LOGD("Try to close transport (%s)",
    // axis_string_get_raw_str(self->base.name));

    if (self->server) {
      // Close the PIPE server asynchronously.
      uv_close((uv_handle_t *)self->server, on_pipe_server_closed);
    } else {
      // Proceed the closing flow synchronously.
      axis_transportbackend_pipe_on_close(self);
    }
  }
}

static void on_server_connected(uv_connect_t *req, int status) {
  assert(req);

  axis_stream_t *stream = (axis_stream_t *)req->data;
  assert(stream && axis_stream_check_integrity(stream));

  free(req);

  axis_transport_t *transport = stream->transport;
  assert(transport);

  if (status < 0) {
    // axis_LOGE("Status = %d connect callback, treat as fail", status);
    goto error;
  }

  axis_streambackend_pipe_t *pipe_stream =
      (axis_streambackend_pipe_t *)stream->backend;
  assert(pipe_stream);

  if (transport && transport->on_server_connected) {
    transport->on_server_connected(transport, stream, status);
  }

  // FALLTHROUGH label
  // The transport is just for connection, not a server-type transport (i.e., a
  // transport with a listening port), so this transport is useless, close it
  // now.

error:
  axis_transport_close(transport);
}

static int axis_transportbackend_pipe_connect(axis_transportbackend_t *backend,
                                             const axis_string_t *dest) {
  axis_string_t *host = NULL;
  axis_stream_t *stream = NULL;

  if (!backend || !dest || axis_string_is_empty(dest)) {
    // axis_LOGE("Empty handle, treat as fail");
    goto error;
  }

  host = __get_pipe_name(dest);
  if (!host) {
    // axis_LOGE("Can not get host info from uri %s", dest->buf);
    goto error;
  }

  stream =
      axis_stream_pipe_create_uv(axis_runloop_get_raw(backend->transport->loop));
  assert(stream);
  stream->transport = backend->transport;

  uv_connect_t *req = (uv_connect_t *)malloc(sizeof(*req));
  if (!req) {
    // axis_LOGE("Not enough memory");
    goto error;
  }

  memset(req, 0, sizeof(*req));

  axis_streambackend_pipe_t *pipe_stream =
      (axis_streambackend_pipe_t *)stream->backend;
  assert(pipe_stream);

  req->data = stream;
  uv_pipe_connect(req, pipe_stream->uv_stream, host->buf, on_server_connected);

  axis_string_destroy(host);

  return 0;

error:
  if (host) {
    axis_string_destroy(host);
  }

  if (stream) {
    axis_stream_close(stream);
  }

  return -1;
}

static void on_client_connected(uv_stream_t *server, int status) {
  if (!server || !server->data) {
    // axis_LOGE("Empty handle, treat as fail");
    goto error;
  }

  if (status < 0) {
    // axis_LOGE("status = %d, treat as fail", status);
    goto error;
  }

  axis_transportbackend_pipe_t *backend =
      (axis_transportbackend_pipe_t *)server->data;
  axis_transport_t *transport = backend->base.transport;

  axis_stream_t *stream =
      axis_stream_pipe_create_uv(axis_runloop_get_raw(transport->loop));
  stream->transport = transport;

  int rc = uv_accept(
      server,
      (uv_stream_t *)((axis_streambackend_pipe_t *)stream->backend)->uv_stream);
  if (rc != 0) {
    // axis_LOGE("uv_accept return %d", rc);
    goto error;
  }

  if (transport->on_client_accepted) {
    transport->on_client_accepted(transport, stream, status);
  }

  return;

error:
  if (stream) {
    axis_stream_close(stream);
  }
}

static int axis_transportbackend_pipe_listen(axis_transportbackend_t *backend,
                                            const axis_string_t *dest) {
  axis_transportbackend_pipe_t *self = (axis_transportbackend_pipe_t *)backend;

  if (self->server) {
    // axis_LOGD("Listen pipe already available");
    return -1;
  }

  uv_pipe_t *server = (uv_pipe_t *)malloc(sizeof(uv_pipe_t));
  assert(server);
  memset(server, 0, sizeof(uv_pipe_t));

  uv_pipe_init(axis_runloop_get_raw(backend->transport->loop), server, 0);
  server->data = backend;
  self->server = server;

  axis_string_t *host = __get_pipe_name(dest);
  assert(host);

  int rc = uv_pipe_bind(server, host->buf);
  if (rc != 0) {
    // axis_LOGD("uv_pipe_bind %s return %d", host->buf, rc);
  }

  rc = uv_listen((uv_stream_t *)server, 128, on_client_connected);
  if (rc != 0) {
    // axis_LOGD("uv_listen %s return %d", host->buf, rc);
  }

  axis_string_destroy(host);

  return rc;
}

static axis_transportbackend_t *axis_transportbackend_pipe_create(
    axis_transport_t *transport, const axis_string_t *name) {
  axis_transportbackend_pipe_t *self = NULL;

  if (!name || !name->buf || !*name->buf) {
    // axis_LOGE("Empty handle, treat as fail");
    goto error;
  }

  self = (axis_transportbackend_pipe_t *)malloc(
      sizeof(axis_transportbackend_pipe_t));
  if (!self) {
    // axis_LOGE("Not enough memory");
    goto error;
  }

  memset(self, 0, sizeof(axis_transportbackend_pipe_t));

  axis_transportbackend_init(&self->base, transport, name);
  self->server = NULL;
  self->base.connect = axis_transportbackend_pipe_connect;
  self->base.listen = axis_transportbackend_pipe_listen;
  self->base.close = axis_transportbackend_pipe_close;

  return (axis_transportbackend_t *)self;

error:
  axis_transportbackend_pipe_destroy(self);
  return NULL;
}

const axis_transportbackend_factory_t uv_tp_backend_pipe = {
    .create = axis_transportbackend_pipe_create,
};
