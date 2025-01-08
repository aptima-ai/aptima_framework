//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_utils/io/general/transport/backend/uv/stream/tcp.h"

#include <stdlib.h>
#include <uv.h>

#include "include_internal/axis_utils/io/transport.h"
#include "axis_utils/io/general/transport/backend/base.h"
#include "axis_utils/io/runloop.h"
#include "axis_utils/io/stream.h"
#include "axis_utils/lib/alloc.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/lib/uri.h"
#include "axis_utils/macro/mark.h"

typedef struct axis_transportbackend_tcp_t {
  axis_transportbackend_t base;

  uv_stream_t *server;
} axis_transportbackend_tcp_t;

// Destroy all the resources hold by tpbackend object. Call this only when
// all the closing flow is completed.
static void axis_transportbackend_tcp_destroy(axis_transportbackend_tcp_t *self) {
  if (!self) {
    return;
  }

  axis_transportbackend_deinit(&self->base);
  axis_FREE(self);
}

static void axis_transportbackend_tcp_on_close(
    axis_transportbackend_tcp_t *self) {
  axis_ASSERT(self, "Invalid argument.");

  axis_transport_t *transport = self->base.transport;
  axis_ASSERT(transport, "Invalid argument.");

  axis_transport_on_close(transport);

  axis_transportbackend_tcp_destroy(self);
}

static void on_tcp_server_closed(uv_handle_t *handle) {
  axis_ASSERT(handle && handle->data, "Invalid argument.");

  axis_transportbackend_tcp_t *self = (axis_transportbackend_tcp_t *)handle->data;
  axis_FREE(handle);

  // Proceed the closing flow.
  axis_transportbackend_tcp_on_close(self);
}

// Trigger the closing flow.
static void axis_transportbackend_tcp_close(axis_transportbackend_t *backend) {
  axis_transportbackend_tcp_t *self = (axis_transportbackend_tcp_t *)backend;

  if (!self) {
    // axis_LOGE("Empty handle, treat as fail");
    return;
  }

  if (axis_atomic_bool_compare_swap(&self->base.is_close, 0, 1)) {
    // axis_LOGD("Try to close transport (%s)",
    // axis_string_get_raw_str(self->base.name));

    if (self->server) {
      // Close the TCP server asynchronously.
      uv_close((uv_handle_t *)self->server, on_tcp_server_closed);
    } else {
      // Proceed the closing flow synchronously.
      axis_transportbackend_tcp_on_close(self);
    }
  }
}

static void on_server_connected(uv_connect_t *req, int status) {
  axis_ASSERT(req, "Invalid argument.");

  axis_stream_t *stream = (axis_stream_t *)req->data;
  axis_ASSERT(stream && axis_stream_check_integrity(stream), "Invalid argument.");

  axis_FREE(req);

  axis_transport_t *transport = stream->transport;
  axis_ASSERT(transport, "Invalid argument.");

  // No matter success or failed, trigger the callback the notify the status to
  // the original requester. Because the original requester would do some
  // cleanup when the connection failed.
  if (transport && transport->on_server_connected) {
    transport->on_server_connected(transport, stream, status);
  }

  // The transport is just for connection, not a server-type transport (i.e., a
  // transport with a listening port), so this transport is useless, close it
  // now.
  axis_transport_close(transport);

  if (status < 0) {
    // axis_LOGE("Status = %d connect callback, treat as fail", status);
  } else {
    axis_streambackend_tcp_t *tcp_stream =
        (axis_streambackend_tcp_t *)stream->backend;
    axis_ASSERT(tcp_stream, "Invalid argument.");

    axis_UNUSED int rc = uv_tcp_keepalive(tcp_stream->uv_stream, 1, 60);
    if (rc != 0) {
      axis_ASSERT(0, "uv_tcp_keepalive() failed: %d", rc);
      // axis_LOGW("uv_tcp_keepalive return %d", rc);
    }
  }
}

static int axis_transportbackend_tcp_connect(axis_transportbackend_t *backend,
                                            const axis_string_t *dest) {
  axis_string_t *host = NULL;
  axis_stream_t *stream = NULL;

  if (!backend || !dest || axis_string_is_empty(dest)) {
    // axis_LOGE("Empty handle, treat as fail");
    goto error;
  }

  host = axis_uri_get_host(axis_string_get_raw_str(dest));
  if (!host) {
    // axis_LOGE("Can not get ip info from uri %s", dest->buf);
    goto error;
  }

  uint16_t port = axis_uri_get_port(axis_string_get_raw_str(dest));
  if (port == 0) {
    // axis_LOGE("Can not get port info from uri %s", dest->buf);
    goto error;
  }

  stream =
      axis_stream_tcp_create_uv(axis_runloop_get_raw(backend->transport->loop));
  axis_ASSERT(stream, "Invalid argument.");
  stream->transport = backend->transport;

  uv_connect_t *req = (uv_connect_t *)axis_MALLOC(sizeof(*req));
  axis_ASSERT(req, "Failed to allocate memory.");
  if (!req) {
    // axis_LOGE("Not enough memory");
    goto error;
  }

  memset(req, 0, sizeof(*req));

  struct sockaddr_in addr = {0};
  req->data = stream;
  axis_UNUSED int rc = uv_ip4_addr(axis_string_get_raw_str(host), port, &addr);
  if (rc != 0) {
    // axis_LOGE("uv_ip4_addr return %d", rc);
    axis_ASSERT(0, "uv_ip4_addr() failed: %d", rc);
    goto error;
  }

  axis_string_destroy(host);
  host = NULL;

  axis_streambackend_tcp_t *tcp_stream =
      (axis_streambackend_tcp_t *)stream->backend;
  axis_ASSERT(tcp_stream, "Invalid argument.");

  rc = uv_tcp_connect(req, tcp_stream->uv_stream, (struct sockaddr *)&addr,
                      on_server_connected);
  if (rc != 0) {
    // axis_LOGE("uv_tcp_connect return %d", rc);
    axis_ASSERT(0, "uv_tcp_connect() failed: %d", rc);
    goto error;
  }

  return rc;

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

  axis_transportbackend_tcp_t *backend =
      (axis_transportbackend_tcp_t *)server->data;
  axis_transport_t *transport = backend->base.transport;

  axis_stream_t *stream =
      axis_stream_tcp_create_uv(axis_runloop_get_raw(transport->loop));
  stream->transport = transport;

  uv_stream_t *uv_stream =
      ((axis_streambackend_tcp_t *)stream->backend)->uv_stream;

  int rc = uv_accept(server, uv_stream);
  if (rc != 0) {
    // axis_LOGE("uv_accept return %d", rc);
    axis_ASSERT(0, "uv_accept() failed: %d", rc);
    goto error;
  }

  axis_streambackend_tcp_dump_info((axis_streambackend_tcp_t *)stream->backend,
                                  "uv_accept() tcp stream: (^1:^2)");

  if (transport->on_client_accepted) {
    transport->on_client_accepted(transport, stream, status);
  }

  return;

error:
  if (stream) {
    axis_stream_close(stream);
  }
}

static int axis_transportbackend_tcp_listen(axis_transportbackend_t *backend,
                                           const axis_string_t *dest) {
  axis_transportbackend_tcp_t *self = (axis_transportbackend_tcp_t *)backend;

  if (self->server) {
    // axis_LOGE("Listen socket already available");
    return -1;
  }

  uv_tcp_t *server = (uv_tcp_t *)axis_MALLOC(sizeof(uv_tcp_t));
  axis_ASSERT(server, "Failed to allocate memory.");

  memset(server, 0, sizeof(uv_tcp_t));

  int rc = uv_tcp_init(axis_runloop_get_raw(backend->transport->loop), server);
  axis_ASSERT(!rc, "uv_tcp_init() failed: %d", rc);

  server->data = backend;
  self->server = server;

  axis_string_t *host = axis_uri_get_host(axis_string_get_raw_str(dest));
  uint16_t port = axis_uri_get_port(axis_string_get_raw_str(dest));
  struct sockaddr_in addr = {0};
  rc = uv_ip4_addr(axis_string_get_raw_str(host), port, &addr);
  if (rc != 0) {
    // axis_LOGE("uv_ip4_addr return %d", rc);
    axis_ASSERT(!rc, "uv_ip4_addr() failed: %d", rc);
  }

  rc = uv_tcp_bind(server, (const struct sockaddr *)&addr, 0);
  if (rc != 0) {
    // axis_LOGE("uv_tcp_bind return %d", rc);
    axis_ASSERT(!rc, "uv_tcp_bind() failed: %d", rc);
  }

  rc = uv_listen((uv_stream_t *)server, 8192, on_client_connected);
  if (rc != 0) {
    // axis_LOGE("uv_listen return %d", rc);
    axis_ASSERT(!rc, "uv_listen() failed: %d", rc);
  }

  axis_string_destroy(host);

  return rc;
}

static axis_transportbackend_t *axis_transportbackend_tcp_create(
    axis_transport_t *transport, const axis_string_t *name) {
  axis_transportbackend_tcp_t *self = NULL;

  if (!name || !name->buf || !*name->buf) {
    // axis_LOGE("Empty handle, treat as fail");
    goto error;
  }

  self = (axis_transportbackend_tcp_t *)axis_MALLOC(
      sizeof(axis_transportbackend_tcp_t));
  axis_ASSERT(self, "Failed to allocate memory.");
  if (!self) {
    // axis_LOGE("Not enough memory");
    goto error;
  }

  memset(self, 0, sizeof(axis_transportbackend_tcp_t));

  axis_transportbackend_init(&self->base, transport, name);
  self->server = NULL;
  self->base.connect = axis_transportbackend_tcp_connect;
  self->base.listen = axis_transportbackend_tcp_listen;
  self->base.close = axis_transportbackend_tcp_close;

  return (axis_transportbackend_t *)self;

error:
  axis_transportbackend_tcp_destroy(self);
  return NULL;
}

const axis_transportbackend_factory_t uv_tp_backend_tcp = {
    .create = axis_transportbackend_tcp_create,
};
