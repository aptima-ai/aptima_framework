//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include <stdlib.h>

#include "include_internal/axis_utils/io/runloop.h"
#include "include_internal/axis_utils/io/transport.h"
#include "axis_utils/container/list.h"
#include "axis_utils/container/list_node.h"
#include "axis_utils/io/general/transport/backend/base.h"
#include "axis_utils/io/runloop.h"
#include "axis_utils/io/stream.h"
#include "axis_utils/io/transport.h"
#include "axis_utils/lib/atomic.h"
#include "axis_utils/lib/mutex.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/lib/thread_once.h"
#include "axis_utils/macro/field.h"
#include "axis_utils/macro/mark.h"

#define axis_STREAMBACKEND_RAW_SIGNATURE 0x861D0758EA843916U

typedef struct axis_streambackend_raw_t axis_streambackend_raw_t;

typedef struct axis_raw_write_req_t {
  void *buf;
  size_t len;
  axis_runloop_async_t *done_signal;
  void *user_data;
  axis_streambackend_raw_t *raw_stream;
  axis_listnode_t node;
} axis_raw_write_req_t;

typedef struct axis_queue_t {
  axis_list_t list;
  axis_atomic_t size;
  axis_mutex_t *lock;
  int valid;
  axis_runloop_async_t *signal;
} axis_queue_t;

static void axis_queue_process_remaining(axis_stream_t *stream,
                                        axis_queue_t *queue);

static void axis_queue_init(axis_runloop_t *loop, axis_queue_t *queue) {
  (void)loop;
  assert(queue);
  axis_list_init(&queue->list);
  queue->lock = axis_mutex_create();
  queue->signal = axis_runloop_async_create(loop->impl);
  queue->valid = 1;
}

static void axis_queue_deinit(axis_queue_t *queue) {
  assert(queue && queue->valid && queue->lock);

  axis_queue_process_remaining(NULL, queue);

  queue->valid = 0;
  axis_mutex_destroy(queue->lock);
  queue->lock = NULL;
}

typedef struct axis_named_queue_t {
  axis_atomic_t ref_cnt;
  axis_string_t name;
  axis_queue_t endpoint[2];
  axis_listnode_t node;
} axis_named_queue_t;

typedef struct axis_streambackend_raw_t {
  axis_streambackend_t base;
  axis_atomic_t signature;
  axis_queue_t *in;
  axis_queue_t *out;
  axis_runloop_t *worker;
  axis_named_queue_t *queue;
} axis_streambackend_raw_t;

typedef struct axis_delayed_task_t {
  axis_transport_t *transport;
  axis_stream_t *stream;
  int status;
  void (*method)(axis_transport_t *, axis_stream_t *, int);
  int close_after_done;
  axis_listnode_t node;
} axis_delayed_task_t;
typedef struct axis_transportbackend_raw_t {
  axis_transportbackend_t base;
  axis_runloop_async_t *delayed_task_signal;
  axis_list_t delayed_tasks;
} axis_transportbackend_raw_t;

static axis_thread_once_t g_init_once = axis_THREAD_ONCE_INIT;
static axis_list_t g_all_streams;
static axis_mutex_t *g_all_streams_lock;

static void axis_init_stream_raw(void) {
  g_all_streams_lock = axis_mutex_create();
  axis_list_init(&g_all_streams);
}

static axis_named_queue_t *axis_find_named_queue_unsafe(
    const axis_string_t *name) {
  axis_named_queue_t *queue = NULL;

  for (axis_listnode_t *itor = axis_list_front(&g_all_streams); itor;
       itor = itor->next) {
    queue = CONTAINER_OF_FROM_FIELD(itor, axis_named_queue_t, node);
    if (axis_string_is_equal(&queue->name, name)) {
      return queue;
    }
  }

  return NULL;
}

static axis_named_queue_t *axis_named_queue_get(axis_runloop_t *loop,
                                              const axis_string_t *name) {
  axis_named_queue_t *queue = NULL;

  axis_UNUSED int rc = axis_mutex_lock(g_all_streams_lock);
  assert(!rc);

  queue = axis_find_named_queue_unsafe(name);
  if (!queue) {
    queue = malloc(sizeof(axis_named_queue_t));
    axis_string_init(&queue->name);
    axis_string_set_formatted(&queue->name, name->buf);
    axis_queue_init(loop, &queue->endpoint[0]);
    axis_queue_init(loop, &queue->endpoint[1]);
    axis_atomic_store(&queue->ref_cnt, 0);
    axis_list_push_back(&g_all_streams, &queue->node);
  }

  axis_atomic_fetch_add(&queue->ref_cnt, 1);

  rc = axis_mutex_unlock(g_all_streams_lock);
  assert(!rc);

  return queue;
}

static void axis_named_queue_put(axis_named_queue_t *queue) {
  axis_UNUSED int rc = axis_mutex_lock(g_all_streams_lock);
  assert(!rc);

  int64_t cnt = axis_atomic_fetch_sub(&queue->ref_cnt, 1);
  if (cnt == 1) {
    axis_list_detach_node(&g_all_streams, &queue->node);
    axis_queue_deinit(&queue->endpoint[0]);
    axis_queue_deinit(&queue->endpoint[1]);
    axis_string_deinit(&queue->name);
    free(queue);
  }

  rc = axis_mutex_unlock(g_all_streams_lock);
  assert(!rc);
}

static void axis_queue_process_remaining(axis_stream_t *stream,
                                        axis_queue_t *queue) {
  axis_list_t tmp = axis_LIST_INIT_VAL;

  axis_UNUSED int rc = axis_mutex_lock(queue->lock);
  assert(!rc);

  while (!axis_list_is_empty(&queue->list)) {
    axis_listnode_t *node = axis_list_pop_front(&queue->list);
    axis_list_push_back(&tmp, node);
  }
  queue->size = 0;

  rc = axis_mutex_unlock(queue->lock);
  assert(!rc);

  while (!axis_list_is_empty(&tmp)) {
    axis_listnode_t *node = axis_list_pop_front(&tmp);
    axis_raw_write_req_t *req =
        CONTAINER_OF_FROM_FIELD(node, axis_raw_write_req_t, node);

    if (stream && stream->on_message_read) {
      stream->on_message_read(stream, req->buf, req->len);
    }

    // notify writer one write request done
    axis_runloop_async_notify(req->done_signal);
  }
}

static void process_delayed_tasks(axis_transportbackend_raw_t *self) {
  axis_list_t tasks_needs_close = axis_LIST_INIT_VAL;

  while (!axis_list_is_empty(&self->delayed_tasks)) {
    axis_listnode_t *node = axis_list_pop_front(&self->delayed_tasks);
    axis_delayed_task_t *task =
        CONTAINER_OF_FROM_FIELD(node, axis_delayed_task_t, node);
    if (task->method) {
      task->method(task->transport, task->stream, task->status);
    }

    if (task->close_after_done) {
      axis_list_push_back(&tasks_needs_close, node);
    } else {
      free(task);
    }
  }

  while (!axis_list_is_empty(&tasks_needs_close)) {
    axis_listnode_t *node = axis_list_pop_front(&tasks_needs_close);
    axis_delayed_task_t *task =
        CONTAINER_OF_FROM_FIELD(node, axis_delayed_task_t, node);

    axis_transport_close(task->transport);

    free(task);
  }
}

static void on_queue_has_more_data(axis_runloop_async_t *handle) {
  axis_streambackend_raw_t *raw_stream = handle->data;
  axis_queue_process_remaining(raw_stream->base.stream, raw_stream->in);
}

static void on_write_request_closed(axis_runloop_async_t *handle) {
  axis_raw_write_req_t *req = handle->data;
  free(req);
  axis_runloop_async_destroy(handle);
}

static void on_write_request_finish(axis_runloop_async_t *handle) {
  axis_raw_write_req_t *req = handle->data;
  axis_streambackend_raw_t *backend = req->raw_stream;

  if (backend->base.stream->on_message_sent) {
    backend->base.stream->on_message_sent(backend->base.stream, 0,
                                          req->user_data);
  }

  if (backend->base.stream->on_message_free) {
    backend->base.stream->on_message_free(backend->base.stream, 0,
                                          req->user_data);
  }

  axis_runloop_async_close(req->done_signal, on_write_request_closed);
}

static void on_stream_in_signal_closed(axis_runloop_async_t *handle) {
  axis_streambackend_raw_t *raw_stream = (axis_streambackend_raw_t *)handle->data;
  axis_named_queue_put(raw_stream->queue);
  axis_streambackend_deinit(&raw_stream->base);

  free(raw_stream);
  axis_runloop_async_destroy(handle);
}

static void axis_streambackend_raw_destroy(axis_streambackend_raw_t *raw_stream) {
  assert(raw_stream);
  axis_runloop_async_close(raw_stream->in->signal, on_stream_in_signal_closed);
}

static int axis_streambackend_raw_start_read(axis_streambackend_t *self) {
  return 0;
}

static int axis_streambackend_raw_stop_read(axis_streambackend_t *self) {
  return 0;
}

static int axis_streambackend_raw_write(axis_streambackend_t *backend,
                                       const void *buf, size_t size,
                                       void *user_data) {
  axis_streambackend_raw_t *raw_stream = (axis_streambackend_raw_t *)backend;
  assert(raw_stream);

  axis_raw_write_req_t *req = malloc(sizeof(axis_raw_write_req_t));
  assert(req);
  req->done_signal = axis_runloop_async_create(raw_stream->base.impl);
  req->buf = (void *)buf;
  req->len = size;
  req->raw_stream = raw_stream;
  req->done_signal->data = req;
  axis_runloop_async_init(req->done_signal, raw_stream->worker,
                         on_write_request_finish);
  req->user_data = user_data;

  axis_UNUSED int rc = axis_mutex_lock(raw_stream->out->lock);
  assert(!rc);

  axis_list_push_back(&raw_stream->out->list, &req->node);
  raw_stream->out->size++;

  rc = axis_mutex_unlock(raw_stream->out->lock);
  assert(!rc);

  // notify reader we have more data
  axis_runloop_async_notify(raw_stream->out->signal);

  return 0;
}

static int axis_streambackend_raw_close(axis_streambackend_t *backend) {
  axis_streambackend_raw_t *raw_stream = (axis_streambackend_raw_t *)backend;
  assert(raw_stream);

  if (axis_atomic_bool_compare_swap(&backend->is_close, 0, 1)) {
    // axis_LOGD("Try to close stream RAW backend.");
    axis_stream_on_close(backend->stream);
    axis_streambackend_raw_destroy(raw_stream);
  }

  return 0;
}

static axis_streambackend_raw_t *axis_streambackend_raw_create(
    const char *impl, axis_stream_t *stream, axis_queue_t *in, axis_queue_t *out) {
  axis_streambackend_raw_t *backend = malloc(sizeof(axis_streambackend_raw_t));
  assert(backend);
  memset(backend, 0, sizeof(axis_streambackend_raw_t));

  axis_streambackend_init(impl, &backend->base, stream);
  axis_atomic_store(&backend->signature, axis_STREAMBACKEND_RAW_SIGNATURE);
  backend->base.start_read = axis_streambackend_raw_start_read;
  backend->base.stop_read = axis_streambackend_raw_stop_read;
  backend->base.write = axis_streambackend_raw_write;
  backend->base.close = axis_streambackend_raw_close;
  backend->in = in;
  backend->out = out;

  return backend;
}

static int axis_transportbackend_new_stream(
    axis_transportbackend_t *backend, const axis_string_t *dest, int in, int out,
    void (*fp)(axis_transport_t *, axis_stream_t *, int), int close_after_done) {
  axis_named_queue_t *queue = NULL;
  axis_stream_t *stream = NULL;
  axis_streambackend_raw_t *streambackend = NULL;
  axis_transportbackend_raw_t *raw_tp = (axis_transportbackend_raw_t *)backend;

  // connect done immediately in raw backend

  queue = axis_named_queue_get(backend->transport->loop, dest);
  if (!queue) {
    // axis_LOGE("Failed to get queue with name %s", dest->buf);
    goto error;
  }

  stream = (axis_stream_t *)malloc(sizeof(*stream));
  if (!stream) {
    // axis_LOGE("Not enough memory, failed to allocate stream");
    goto error;
  }

  memset(stream, 0, sizeof(*stream));
  axis_stream_init(stream);

  streambackend =
      axis_streambackend_raw_create(backend->transport->loop->impl, stream,
                                   &queue->endpoint[in], &queue->endpoint[out]);
  if (!streambackend) {
    // axis_LOGE("Failed to create stream backend");
    goto error;
  }

  streambackend->worker = backend->transport->loop;
  streambackend->queue = queue;
  streambackend->in->signal->data = streambackend;
  axis_runloop_async_init(streambackend->in->signal, streambackend->worker,
                         on_queue_has_more_data);

  axis_delayed_task_t *req = malloc(sizeof(axis_delayed_task_t));
  assert(req);
  req->transport = backend->transport;
  req->stream = stream;
  req->status = 0;
  req->method = fp;
  req->close_after_done = close_after_done;
  axis_list_push_back(&raw_tp->delayed_tasks, &req->node);
  axis_runloop_async_notify(raw_tp->delayed_task_signal);

  return 0;

error:
  if (queue) {
    axis_named_queue_put(queue);
  }

  if (stream) {
    free(stream);
  }

  if (streambackend) {
    axis_streambackend_raw_destroy(streambackend);
  }

  return -1;
}

static int axis_transportbackend_raw_connect(axis_transportbackend_t *backend,
                                            const axis_string_t *dest) {
  if (!backend || !backend->transport || !dest || axis_string_is_empty(dest)) {
    // axis_LOGE("Empty handle, treat as fail");
    return -1;
  }

  // axis_LOGD("Connecting.");
  return axis_transportbackend_new_stream(
      backend, dest, 0, 1, backend->transport->on_server_connected, 1);
}

static int axis_transportbackend_raw_listen(axis_transportbackend_t *backend,
                                           const axis_string_t *dest) {
  if (!backend || !backend->transport || !dest || axis_string_is_empty(dest)) {
    // axis_LOGE("Empty handle, treat as fail");
    return -1;
  }

  // axis_LOGD("Listening.");
  return axis_transportbackend_new_stream(
      backend, dest, 1, 0, backend->transport->on_client_accepted, 0);
}

static void on_delayed_task_signal_closed(axis_runloop_async_t *handle) {
  axis_transportbackend_raw_t *self = (axis_transportbackend_raw_t *)handle->data;
  axis_transport_t *transport = self->base.transport;
  self->delayed_task_signal->data = NULL;
  axis_transport_on_close(transport);
  axis_transportbackend_deinit(&self->base);
  free(self);
  axis_runloop_async_destroy(handle);
}

static void axis_transportbackend_raw_close(axis_transportbackend_t *backend) {
  axis_transportbackend_raw_t *self = (axis_transportbackend_raw_t *)backend;

  if (!self) {
    // axis_LOGD("empty handle, treat as fail");
    return;
  }

  if (axis_atomic_bool_compare_swap(&self->base.is_close, 0, 1)) {
    // axis_LOGD("Try to close transport (%s)",
    // axis_string_get_raw_str(self->base.name));

    axis_transport_t *transport = self->base.transport;
    assert(transport);
    process_delayed_tasks(self);
    axis_runloop_async_close(self->delayed_task_signal,
                            on_delayed_task_signal_closed);
  }
}

static void on_delayed_task(axis_runloop_async_t *handle) {
  if (!handle || !handle->data) {
    return;
  }

  axis_transportbackend_raw_t *self = handle->data;
  process_delayed_tasks(self);
}

static axis_transportbackend_t *axis_transportbackend_raw_create(
    axis_transport_t *transport, const axis_string_t *name) {
  axis_transportbackend_raw_t *self = NULL;

  axis_thread_once(&g_init_once, axis_init_stream_raw);

  if (!name || !name->buf || !*name->buf) {
    // axis_LOGE("Empty name, treat as fail");
    goto error;
  }

  self =
      (axis_transportbackend_raw_t *)malloc(sizeof(axis_transportbackend_raw_t));
  if (!self) {
    // axis_LOGE("Not enough memory");
    goto error;
  }

  memset(self, 0, sizeof(axis_transportbackend_raw_t));

  axis_transportbackend_init(&self->base, transport, name);
  self->base.connect = axis_transportbackend_raw_connect;
  self->base.listen = axis_transportbackend_raw_listen;
  self->base.close = axis_transportbackend_raw_close;
  axis_list_init(&self->delayed_tasks);
  self->delayed_task_signal = axis_runloop_async_create(transport->loop->impl);
  self->delayed_task_signal->data = self;
  axis_runloop_async_init(self->delayed_task_signal, transport->loop,
                         on_delayed_task);

  return (axis_transportbackend_t *)self;

error:
  axis_transportbackend_raw_close(&self->base);
  return NULL;
};

const axis_transportbackend_factory_t general_tp_backend_raw = {
    .create = axis_transportbackend_raw_create};
