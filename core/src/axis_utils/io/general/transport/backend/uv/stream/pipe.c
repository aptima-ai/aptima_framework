//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_utils/io/general/transport/backend/uv/stream/pipe.h"

#include <stdarg.h>
#include <stdlib.h>
#include <uv.h>

#include "axis_utils/io/general/transport/backend/base.h"
#include "axis_utils/io/stream.h"
#include "axis_utils/io/transport.h"
#include "axis_utils/macro/mark.h"

// Message write structure
typedef struct axis_uv_write_req_t {
  uv_write_t req;

  void *user_data;
} axis_uv_write_req_t;

static bool axis_streambackend_pipe_check_integrity(
    axis_streambackend_pipe_t *self) {
  assert(self);
  if (axis_atomic_load(&self->signature) != axis_STREAMBACKEND_PIPE_SIGNATURE) {
    return false;
  }
  return true;
}

static void on_pipe_alloc(uv_handle_t *uv_handle, size_t suggested_size,
                          uv_buf_t *buf) {
  assert(uv_handle && suggested_size && buf);

  buf->base = malloc(suggested_size);
  assert(buf->base);

  buf->len = suggested_size;
}

static void on_pipe_read(uv_stream_t *uv_stream, ssize_t nread,
                         const uv_buf_t *buf) {
  assert(uv_stream && uv_stream->data);

  axis_streambackend_pipe_t *pipe_stream = uv_stream->data;
  assert(pipe_stream && axis_streambackend_pipe_check_integrity(pipe_stream));

  axis_stream_t *stream = pipe_stream->base.stream;
  assert(stream && axis_stream_check_integrity(stream));

  if (nread == 0) {
    // Nothing to read.
  } else if (nread < 0) {
    // Something bad happened, free the message.
    free(buf->base);

    // Notify that there is something bad happened.
    if (stream->on_message_read) {
      stream->on_message_read(stream, NULL, (int)nread);
    }
  } else {
    if (stream->on_message_read) {
      stream->on_message_read(stream, buf->base, (int)nread);
    }
    free(buf->base);
  }
}

static int axis_streambackend_pipe_start_read(axis_streambackend_t *self_) {
  axis_streambackend_pipe_t *pipe_stream = (axis_streambackend_pipe_t *)self_;
  if (!pipe_stream) {
    // axis_LOGE("Empty handle, treat as fail");
    return -1;
  }
  assert(axis_streambackend_pipe_check_integrity(pipe_stream));

  if (!pipe_stream->uv_stream) {
    // axis_LOGE("Empty uv handle, treat as fail");
    return -1;
  }

  int rc = uv_read_start((uv_stream_t *)pipe_stream->uv_stream, on_pipe_alloc,
                         on_pipe_read);
  if (rc != 0) {
    // axis_LOGD("uv_read_start return %d", rc);
  }

  return rc;
}

static int axis_streambackend_pipe_stop_read(axis_streambackend_t *self_) {
  axis_streambackend_pipe_t *pipe_stream = (axis_streambackend_pipe_t *)self_;
  if (!pipe_stream) {
    // axis_LOGE("Empty handle, treat as fail");
    return -1;
  }
  assert(axis_streambackend_pipe_check_integrity(pipe_stream));

  if (!pipe_stream->uv_stream) {
    // axis_LOGE("Empty uv handle, treat as fail");
    return -1;
  }

  int rc = uv_read_stop((uv_stream_t *)pipe_stream->uv_stream);
  if (rc != 0) {
    // axis_LOGW("uv_read_stop return %d", rc);
  }

  return rc;
}

static void on_pipe_write_done(uv_write_t *wreq, axis_UNUSED int status) {
  axis_uv_write_req_t *req = (axis_uv_write_req_t *)wreq;

  axis_streambackend_pipe_t *pipe_stream =
      (axis_streambackend_pipe_t *)wreq->data;
  assert(pipe_stream && axis_streambackend_pipe_check_integrity(pipe_stream));

  axis_stream_t *stream = pipe_stream->base.stream;
  assert(stream && axis_stream_check_integrity(stream));

  if (stream->on_message_sent) {
    // Call the callback function.
    stream->on_message_sent(stream, status, req->user_data);
  }

  if (stream->on_message_free) {
    // Release the message data.
    stream->on_message_free(stream, status, req->user_data);
  }

  // Release the write request.
  free(req);
}

static int axis_streambackend_pipe_write(axis_streambackend_t *backend,
                                        const void *msg, size_t size,
                                        void *user_data) {
  axis_streambackend_pipe_t *pipe_stream = (axis_streambackend_pipe_t *)backend;
  assert(pipe_stream);

  axis_uv_write_req_t *req = malloc(sizeof(axis_uv_write_req_t));
  assert(req);
  req->req.data = pipe_stream;
  req->user_data = user_data;

  uv_buf_t buf = uv_buf_init((char *)msg, size);

  int rc = uv_write((uv_write_t *)req, (uv_stream_t *)pipe_stream->uv_stream,
                    &buf, 1, on_pipe_write_done);
  if (rc != 0) {
    // axis_LOGW("uv_write return %d", rc);
  }

  return rc;
}

static void axis_streambackend_pipe_destroy(
    axis_streambackend_pipe_t *pipe_stream) {
  assert(pipe_stream && axis_streambackend_pipe_check_integrity(pipe_stream) &&
         pipe_stream->uv_stream);

  axis_streambackend_deinit(&pipe_stream->base);

  free(pipe_stream->uv_stream);
  free(pipe_stream);
}

static void axis_streambackend_pipe_on_close(uv_handle_t *uv_handle) {
  assert(uv_handle && uv_handle->data);

  // axis_LOGD("Close stream.");

  axis_streambackend_pipe_t *pipe_stream =
      (axis_streambackend_pipe_t *)uv_handle->data;
  assert(pipe_stream && axis_streambackend_pipe_check_integrity(pipe_stream));

  axis_stream_t *stream = pipe_stream->base.stream;
  assert(stream && axis_stream_check_integrity(stream));

  axis_stream_on_close(stream);
  axis_streambackend_pipe_destroy(pipe_stream);
}

static int axis_streambackend_pipe_close(axis_streambackend_t *backend) {
  axis_streambackend_pipe_t *pipe_stream = (axis_streambackend_pipe_t *)backend;
  assert(pipe_stream && axis_streambackend_pipe_check_integrity(pipe_stream));

  if (axis_atomic_bool_compare_swap(&backend->is_close, 0, 1)) {
    // axis_LOGD("Try to close stream PIPE backend.");
    uv_close((uv_handle_t *)pipe_stream->uv_stream,
             axis_streambackend_pipe_on_close);
  }

  return 0;
}

static axis_streambackend_pipe_t *axis_streambackend_pipe_create(
    axis_stream_t *stream) {
  assert(stream);

  axis_streambackend_pipe_t *pipe_stream =
      (axis_streambackend_pipe_t *)malloc(sizeof(axis_streambackend_pipe_t));
  assert(pipe_stream);
  memset(pipe_stream, 0, sizeof(axis_streambackend_pipe_t));

  axis_streambackend_init(axis_RUNLOOP_UV, &pipe_stream->base, stream);
  axis_atomic_store(&pipe_stream->signature, axis_STREAMBACKEND_PIPE_SIGNATURE);

  pipe_stream->base.start_read = axis_streambackend_pipe_start_read;
  pipe_stream->base.stop_read = axis_streambackend_pipe_stop_read;
  pipe_stream->base.write = axis_streambackend_pipe_write;
  pipe_stream->base.close = axis_streambackend_pipe_close;

  pipe_stream->uv_stream = (uv_pipe_t *)malloc(sizeof(uv_pipe_t));
  assert(pipe_stream->uv_stream);
  memset(pipe_stream->uv_stream, 0, sizeof(uv_pipe_t));

  pipe_stream->uv_stream->data = pipe_stream;

  return pipe_stream;
}

axis_stream_t *axis_stream_pipe_create_uv(uv_loop_t *loop) {
  axis_stream_t *stream = (axis_stream_t *)malloc(sizeof(*stream));
  assert(stream);
  memset(stream, 0, sizeof(*stream));
  axis_stream_init(stream);

  axis_streambackend_pipe_t *pipe_stream = axis_streambackend_pipe_create(stream);

  int rc = uv_pipe_init(loop, pipe_stream->uv_stream, 0);
  if (rc != 0) {
    // axis_LOGD("uv_pipe_init return %d", rc);
    goto error;
  }
  return stream;

error:
  if (stream) {
    axis_stream_close(stream);
  }
  return NULL;
}
