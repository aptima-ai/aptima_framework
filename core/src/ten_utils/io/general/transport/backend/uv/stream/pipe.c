//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include "ten_utils/io/general/transport/backend/uv/stream/pipe.h"

#include <stdarg.h>
#include <stdlib.h>
#include <uv.h>

#include "ten_utils/io/general/transport/backend/base.h"
#include "ten_utils/io/stream.h"
#include "ten_utils/io/transport.h"
#include "ten_utils/lib/string.h"
#include "ten_utils/lib/uri.h"
#include "ten_utils/macro/macros.h"

// Message write structure
typedef struct ten_uv_write_req_t {
  uv_write_t req;

  void *user_data;
} ten_uv_write_req_t;

static bool ten_streambackend_pipe_check_integrity(
    ten_streambackend_pipe_t *self) {
  assert(self);
  if (ten_atomic_load(&self->signature) != TEN_STREAMBACKEND_PIPE_SIGNATURE) {
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

  ten_streambackend_pipe_t *pipe_stream = uv_stream->data;
  assert(pipe_stream && ten_streambackend_pipe_check_integrity(pipe_stream));

  ten_stream_t *stream = pipe_stream->base.stream;
  assert(stream && ten_stream_check_integrity(stream));

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

static int ten_streambackend_pipe_start_read(ten_streambackend_t *self_) {
  ten_streambackend_pipe_t *pipe_stream = (ten_streambackend_pipe_t *)self_;
  if (!pipe_stream) {
    // TEN_LOGE("Empty handle, treat as fail");
    return -1;
  }
  assert(ten_streambackend_pipe_check_integrity(pipe_stream));

  if (!pipe_stream->uv_stream) {
    // TEN_LOGE("Empty uv handle, treat as fail");
    return -1;
  }

  int rc = uv_read_start((uv_stream_t *)pipe_stream->uv_stream, on_pipe_alloc,
                         on_pipe_read);
  if (rc != 0) {
    // TEN_LOGD("uv_read_start return %d", rc);
  }

  return rc;
}

static int ten_streambackend_pipe_stop_read(ten_streambackend_t *self_) {
  ten_streambackend_pipe_t *pipe_stream = (ten_streambackend_pipe_t *)self_;
  if (!pipe_stream) {
    // TEN_LOGE("Empty handle, treat as fail");
    return -1;
  }
  assert(ten_streambackend_pipe_check_integrity(pipe_stream));

  if (!pipe_stream->uv_stream) {
    // TEN_LOGE("Empty uv handle, treat as fail");
    return -1;
  }

  int rc = uv_read_stop((uv_stream_t *)pipe_stream->uv_stream);
  if (rc != 0) {
    // TEN_LOGW("uv_read_stop return %d", rc);
  }

  return rc;
}

static void on_pipe_write_done(uv_write_t *wreq, TEN_UNUSED int status) {
  ten_uv_write_req_t *req = (ten_uv_write_req_t *)wreq;

  ten_streambackend_pipe_t *pipe_stream =
      (ten_streambackend_pipe_t *)wreq->data;
  assert(pipe_stream && ten_streambackend_pipe_check_integrity(pipe_stream));

  ten_stream_t *stream = pipe_stream->base.stream;
  assert(stream && ten_stream_check_integrity(stream));

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

static int ten_streambackend_pipe_write(ten_streambackend_t *backend,
                                        const void *msg, size_t size,
                                        void *user_data) {
  ten_streambackend_pipe_t *pipe_stream = (ten_streambackend_pipe_t *)backend;
  assert(pipe_stream);

  ten_uv_write_req_t *req = malloc(sizeof(ten_uv_write_req_t));
  assert(req);
  req->req.data = pipe_stream;
  req->user_data = user_data;

  uv_buf_t buf = uv_buf_init((char *)msg, size);

  int rc = uv_write((uv_write_t *)req, (uv_stream_t *)pipe_stream->uv_stream,
                    &buf, 1, on_pipe_write_done);
  if (rc != 0) {
    // TEN_LOGW("uv_write return %d", rc);
  }

  return rc;
}

static void ten_streambackend_pipe_destroy(
    ten_streambackend_pipe_t *pipe_stream) {
  assert(pipe_stream && ten_streambackend_pipe_check_integrity(pipe_stream) &&
         pipe_stream->uv_stream);

  ten_streambackend_deinit(&pipe_stream->base);

  free(pipe_stream->uv_stream);
  free(pipe_stream);
}

static void ten_streambackend_pipe_on_close(uv_handle_t *uv_handle) {
  assert(uv_handle && uv_handle->data);

  // TEN_LOGD("Close stream.");

  ten_streambackend_pipe_t *pipe_stream =
      (ten_streambackend_pipe_t *)uv_handle->data;
  assert(pipe_stream && ten_streambackend_pipe_check_integrity(pipe_stream));

  ten_stream_t *stream = pipe_stream->base.stream;
  assert(stream && ten_stream_check_integrity(stream));

  ten_stream_on_close(stream);
  ten_streambackend_pipe_destroy(pipe_stream);
}

static int ten_streambackend_pipe_close(ten_streambackend_t *backend) {
  ten_streambackend_pipe_t *pipe_stream = (ten_streambackend_pipe_t *)backend;
  assert(pipe_stream && ten_streambackend_pipe_check_integrity(pipe_stream));

  if (ten_atomic_bool_compare_swap(&backend->is_close, 0, 1)) {
    // TEN_LOGD("Try to close stream PIPE backend.");
    uv_close((uv_handle_t *)pipe_stream->uv_stream,
             ten_streambackend_pipe_on_close);
  }

  return 0;
}

static ten_streambackend_pipe_t *ten_streambackend_pipe_create(
    ten_stream_t *stream) {
  assert(stream);

  ten_streambackend_pipe_t *pipe_stream =
      (ten_streambackend_pipe_t *)malloc(sizeof(ten_streambackend_pipe_t));
  assert(pipe_stream);
  memset(pipe_stream, 0, sizeof(ten_streambackend_pipe_t));

  ten_streambackend_init(TEN_RUNLOOP_UV, &pipe_stream->base, stream);
  ten_atomic_store(&pipe_stream->signature, TEN_STREAMBACKEND_PIPE_SIGNATURE);

  pipe_stream->base.start_read = ten_streambackend_pipe_start_read;
  pipe_stream->base.stop_read = ten_streambackend_pipe_stop_read;
  pipe_stream->base.write = ten_streambackend_pipe_write;
  pipe_stream->base.close = ten_streambackend_pipe_close;

  pipe_stream->uv_stream = (uv_pipe_t *)malloc(sizeof(uv_pipe_t));
  assert(pipe_stream->uv_stream);
  memset(pipe_stream->uv_stream, 0, sizeof(uv_pipe_t));

  pipe_stream->uv_stream->data = pipe_stream;

  return pipe_stream;
}

ten_stream_t *ten_stream_pipe_create_uv(uv_loop_t *loop) {
  ten_stream_t *stream = (ten_stream_t *)malloc(sizeof(*stream));
  assert(stream);
  memset(stream, 0, sizeof(*stream));
  ten_stream_init(stream);

  ten_streambackend_pipe_t *pipe_stream = ten_streambackend_pipe_create(stream);

  int rc = uv_pipe_init(loop, pipe_stream->uv_stream, 0);
  if (rc != 0) {
    // TEN_LOGD("uv_pipe_init return %d", rc);
    goto error;
  }
  return stream;

error:
  if (stream) {
    ten_stream_close(stream);
  }
  return NULL;
}
