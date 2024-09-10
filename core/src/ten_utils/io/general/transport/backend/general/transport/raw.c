//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include <stdlib.h>

#include "include_internal/ten_utils/io/runloop.h"
#include "include_internal/ten_utils/io/transport.h"
#include "ten_utils/container/list.h"
#include "ten_utils/container/list_node.h"
#include "ten_utils/io/general/transport/backend/base.h"
#include "ten_utils/io/runloop.h"
#include "ten_utils/io/stream.h"
#include "ten_utils/io/transport.h"
#include "ten_utils/lib/atomic.h"
#include "ten_utils/lib/mutex.h"
#include "ten_utils/lib/string.h"
#include "ten_utils/lib/thread_once.h"
#include "ten_utils/macro/field.h"

#define TEN_STREAMBACKEND_RAW_SIGNATURE 0x861D0758EA843916U

typedef struct ten_streambackend_raw_t ten_streambackend_raw_t;

typedef struct ten_raw_write_req_t {
  void *buf;
  size_t len;
  ten_runloop_async_t *done_signal;
  void *user_data;
  ten_streambackend_raw_t *raw_stream;
  ten_listnode_t node;
} ten_raw_write_req_t;

typedef struct ten_queue_t {
  ten_list_t list;
  ten_atomic_t size;
  ten_mutex_t *lock;
  int valid;
  ten_runloop_async_t *signal;
} ten_queue_t;

static void ten_queue_process_remaining(ten_stream_t *stream,
                                        ten_queue_t *queue);

static void ten_queue_init(ten_runloop_t *loop, ten_queue_t *queue) {
  (void)loop;
  assert(queue);
  ten_list_init(&queue->list);
  queue->lock = ten_mutex_create();
  queue->signal = ten_runloop_async_create(loop->impl);
  queue->valid = 1;
}

static void ten_queue_deinit(ten_queue_t *queue) {
  assert(queue && queue->valid && queue->lock);

  ten_queue_process_remaining(NULL, queue);

  queue->valid = 0;
  ten_mutex_destroy(queue->lock);
  queue->lock = NULL;
}

typedef struct ten_named_queue_t {
  ten_atomic_t ref_cnt;
  ten_string_t name;
  ten_queue_t endpoint[2];
  ten_listnode_t node;
} ten_named_queue_t;

typedef struct ten_streambackend_raw_t {
  ten_streambackend_t base;
  ten_atomic_t signature;
  ten_queue_t *in;
  ten_queue_t *out;
  ten_runloop_t *worker;
  ten_named_queue_t *queue;
} ten_streambackend_raw_t;

typedef struct ten_delayed_task_t {
  ten_transport_t *transport;
  ten_stream_t *stream;
  int status;
  void (*method)(ten_transport_t *, ten_stream_t *, int);
  int close_after_done;
  ten_listnode_t node;
} ten_delayed_task_t;
typedef struct ten_transportbackend_raw_t {
  ten_transportbackend_t base;
  ten_runloop_async_t *delayed_task_signal;
  ten_list_t delayed_tasks;
} ten_transportbackend_raw_t;

static ten_thread_once_t g_init_once = TEN_THREAD_ONCE_INIT;
static ten_list_t g_all_streams;
static ten_mutex_t *g_all_streams_lock;

static void ten_init_stream_raw(void) {
  g_all_streams_lock = ten_mutex_create();
  ten_list_init(&g_all_streams);
}

static ten_named_queue_t *ten_find_named_queue_unsafe(
    const ten_string_t *name) {
  ten_named_queue_t *queue = NULL;

  for (ten_listnode_t *itor = ten_list_front(&g_all_streams); itor;
       itor = itor->next) {
    queue = CONTAINER_OF_FROM_FIELD(itor, ten_named_queue_t, node);
    if (ten_string_is_equal(&queue->name, name)) {
      return queue;
    }
  }

  return NULL;
}

static ten_named_queue_t *ten_named_queue_get(ten_runloop_t *loop,
                                              const ten_string_t *name) {
  ten_named_queue_t *queue = NULL;

  TEN_UNUSED int rc = ten_mutex_lock(g_all_streams_lock);
  assert(!rc);

  queue = ten_find_named_queue_unsafe(name);
  if (!queue) {
    queue = malloc(sizeof(ten_named_queue_t));
    ten_string_init(&queue->name);
    ten_string_set_formatted(&queue->name, name->buf);
    ten_queue_init(loop, &queue->endpoint[0]);
    ten_queue_init(loop, &queue->endpoint[1]);
    ten_atomic_store(&queue->ref_cnt, 0);
    ten_list_push_back(&g_all_streams, &queue->node);
  }

  ten_atomic_fetch_add(&queue->ref_cnt, 1);

  rc = ten_mutex_unlock(g_all_streams_lock);
  assert(!rc);

  return queue;
}

static void ten_named_queue_put(ten_named_queue_t *queue) {
  TEN_UNUSED int rc = ten_mutex_lock(g_all_streams_lock);
  assert(!rc);

  int64_t cnt = ten_atomic_fetch_sub(&queue->ref_cnt, 1);
  if (cnt == 1) {
    ten_list_detach_node(&g_all_streams, &queue->node);
    ten_queue_deinit(&queue->endpoint[0]);
    ten_queue_deinit(&queue->endpoint[1]);
    ten_string_deinit(&queue->name);
    free(queue);
  }

  rc = ten_mutex_unlock(g_all_streams_lock);
  assert(!rc);
}

static void ten_queue_process_remaining(ten_stream_t *stream,
                                        ten_queue_t *queue) {
  ten_list_t tmp = TEN_LIST_INIT_VAL;

  TEN_UNUSED int rc = ten_mutex_lock(queue->lock);
  assert(!rc);

  while (!ten_list_is_empty(&queue->list)) {
    ten_listnode_t *node = ten_list_pop_front(&queue->list);
    ten_list_push_back(&tmp, node);
  }
  queue->size = 0;

  rc = ten_mutex_unlock(queue->lock);
  assert(!rc);

  while (!ten_list_is_empty(&tmp)) {
    ten_listnode_t *node = ten_list_pop_front(&tmp);
    ten_raw_write_req_t *req =
        CONTAINER_OF_FROM_FIELD(node, ten_raw_write_req_t, node);

    if (stream && stream->on_message_read) {
      stream->on_message_read(stream, req->buf, req->len);
    }

    // notify writer one write request done
    ten_runloop_async_notify(req->done_signal);
  }
}

static void process_delayed_tasks(ten_transportbackend_raw_t *self) {
  ten_list_t tasks_needs_close = TEN_LIST_INIT_VAL;

  while (!ten_list_is_empty(&self->delayed_tasks)) {
    ten_listnode_t *node = ten_list_pop_front(&self->delayed_tasks);
    ten_delayed_task_t *task =
        CONTAINER_OF_FROM_FIELD(node, ten_delayed_task_t, node);
    if (task->method) {
      task->method(task->transport, task->stream, task->status);
    }

    if (task->close_after_done) {
      ten_list_push_back(&tasks_needs_close, node);
    } else {
      free(task);
    }
  }

  while (!ten_list_is_empty(&tasks_needs_close)) {
    ten_listnode_t *node = ten_list_pop_front(&tasks_needs_close);
    ten_delayed_task_t *task =
        CONTAINER_OF_FROM_FIELD(node, ten_delayed_task_t, node);

    ten_transport_close(task->transport);

    free(task);
  }
}

static void on_queue_has_more_data(ten_runloop_async_t *handle) {
  ten_streambackend_raw_t *raw_stream = handle->data;
  ten_queue_process_remaining(raw_stream->base.stream, raw_stream->in);
}

static void on_write_request_closed(ten_runloop_async_t *handle) {
  ten_raw_write_req_t *req = handle->data;
  free(req);
  ten_runloop_async_destroy(handle);
}

static void on_write_request_finish(ten_runloop_async_t *handle) {
  ten_raw_write_req_t *req = handle->data;
  ten_streambackend_raw_t *backend = req->raw_stream;

  if (backend->base.stream->on_message_sent) {
    backend->base.stream->on_message_sent(backend->base.stream, 0,
                                          req->user_data);
  }

  if (backend->base.stream->on_message_free) {
    backend->base.stream->on_message_free(backend->base.stream, 0,
                                          req->user_data);
  }

  ten_runloop_async_close(req->done_signal, on_write_request_closed);
}

static void on_stream_in_signal_closed(ten_runloop_async_t *handle) {
  ten_streambackend_raw_t *raw_stream = (ten_streambackend_raw_t *)handle->data;
  ten_named_queue_put(raw_stream->queue);
  ten_streambackend_deinit(&raw_stream->base);

  free(raw_stream);
  ten_runloop_async_destroy(handle);
}

static void ten_streambackend_raw_destroy(ten_streambackend_raw_t *raw_stream) {
  assert(raw_stream);
  ten_runloop_async_close(raw_stream->in->signal, on_stream_in_signal_closed);
}

static int ten_streambackend_raw_start_read(ten_streambackend_t *self) {
  return 0;
}

static int ten_streambackend_raw_stop_read(ten_streambackend_t *self) {
  return 0;
}

static int ten_streambackend_raw_write(ten_streambackend_t *backend,
                                       const void *buf, size_t size,
                                       void *user_data) {
  ten_streambackend_raw_t *raw_stream = (ten_streambackend_raw_t *)backend;
  assert(raw_stream);

  ten_raw_write_req_t *req = malloc(sizeof(ten_raw_write_req_t));
  assert(req);
  req->done_signal = ten_runloop_async_create(raw_stream->base.impl);
  req->buf = (void *)buf;
  req->len = size;
  req->raw_stream = raw_stream;
  req->done_signal->data = req;
  ten_runloop_async_init(req->done_signal, raw_stream->worker,
                         on_write_request_finish);
  req->user_data = user_data;

  TEN_UNUSED int rc = ten_mutex_lock(raw_stream->out->lock);
  assert(!rc);

  ten_list_push_back(&raw_stream->out->list, &req->node);
  raw_stream->out->size++;

  rc = ten_mutex_unlock(raw_stream->out->lock);
  assert(!rc);

  // notify reader we have more data
  ten_runloop_async_notify(raw_stream->out->signal);

  return 0;
}

static int ten_streambackend_raw_close(ten_streambackend_t *backend) {
  ten_streambackend_raw_t *raw_stream = (ten_streambackend_raw_t *)backend;
  assert(raw_stream);

  if (ten_atomic_bool_compare_swap(&backend->is_close, 0, 1)) {
    // TEN_LOGD("Try to close stream RAW backend.");
    ten_stream_on_close(backend->stream);
    ten_streambackend_raw_destroy(raw_stream);
  }

  return 0;
}

static ten_streambackend_raw_t *ten_streambackend_raw_create(
    const char *impl, ten_stream_t *stream, ten_queue_t *in, ten_queue_t *out) {
  ten_streambackend_raw_t *backend = malloc(sizeof(ten_streambackend_raw_t));
  assert(backend);
  memset(backend, 0, sizeof(ten_streambackend_raw_t));

  ten_streambackend_init(impl, &backend->base, stream);
  ten_atomic_store(&backend->signature, TEN_STREAMBACKEND_RAW_SIGNATURE);
  backend->base.start_read = ten_streambackend_raw_start_read;
  backend->base.stop_read = ten_streambackend_raw_stop_read;
  backend->base.write = ten_streambackend_raw_write;
  backend->base.close = ten_streambackend_raw_close;
  backend->in = in;
  backend->out = out;

  return backend;
}

static int ten_transportbackend_new_stream(
    ten_transportbackend_t *backend, const ten_string_t *dest, int in, int out,
    void (*fp)(ten_transport_t *, ten_stream_t *, int), int close_after_done) {
  ten_named_queue_t *queue = NULL;
  ten_stream_t *stream = NULL;
  ten_streambackend_raw_t *streambackend = NULL;
  ten_transportbackend_raw_t *raw_tp = (ten_transportbackend_raw_t *)backend;

  // connect done immediately in raw backend

  queue = ten_named_queue_get(backend->transport->loop, dest);
  if (!queue) {
    // TEN_LOGE("Failed to get queue with name %s", dest->buf);
    goto error;
  }

  stream = (ten_stream_t *)malloc(sizeof(*stream));
  if (!stream) {
    // TEN_LOGE("Not enough memory, failed to allocate stream");
    goto error;
  }

  memset(stream, 0, sizeof(*stream));
  ten_stream_init(stream);

  streambackend =
      ten_streambackend_raw_create(backend->transport->loop->impl, stream,
                                   &queue->endpoint[in], &queue->endpoint[out]);
  if (!streambackend) {
    // TEN_LOGE("Failed to create stream backend");
    goto error;
  }

  streambackend->worker = backend->transport->loop;
  streambackend->queue = queue;
  streambackend->in->signal->data = streambackend;
  ten_runloop_async_init(streambackend->in->signal, streambackend->worker,
                         on_queue_has_more_data);

  ten_delayed_task_t *req = malloc(sizeof(ten_delayed_task_t));
  assert(req);
  req->transport = backend->transport;
  req->stream = stream;
  req->status = 0;
  req->method = fp;
  req->close_after_done = close_after_done;
  ten_list_push_back(&raw_tp->delayed_tasks, &req->node);
  ten_runloop_async_notify(raw_tp->delayed_task_signal);

  return 0;

error:
  if (queue) {
    ten_named_queue_put(queue);
  }

  if (stream) {
    free(stream);
  }

  if (streambackend) {
    ten_streambackend_raw_destroy(streambackend);
  }

  return -1;
}

static int ten_transportbackend_raw_connect(ten_transportbackend_t *backend,
                                            const ten_string_t *dest) {
  if (!backend || !backend->transport || !dest || ten_string_is_empty(dest)) {
    // TEN_LOGE("Empty handle, treat as fail");
    return -1;
  }

  // TEN_LOGD("Connecting.");
  return ten_transportbackend_new_stream(
      backend, dest, 0, 1, backend->transport->on_server_connected, 1);
}

static int ten_transportbackend_raw_listen(ten_transportbackend_t *backend,
                                           const ten_string_t *dest) {
  if (!backend || !backend->transport || !dest || ten_string_is_empty(dest)) {
    // TEN_LOGE("Empty handle, treat as fail");
    return -1;
  }

  // TEN_LOGD("Listening.");
  return ten_transportbackend_new_stream(
      backend, dest, 1, 0, backend->transport->on_client_accepted, 0);
}

static void on_delayed_task_signal_closed(ten_runloop_async_t *handle) {
  ten_transportbackend_raw_t *self = (ten_transportbackend_raw_t *)handle->data;
  ten_transport_t *transport = self->base.transport;
  self->delayed_task_signal->data = NULL;
  ten_transport_on_close(transport);
  ten_transportbackend_deinit(&self->base);
  free(self);
  ten_runloop_async_destroy(handle);
}

static void ten_transportbackend_raw_close(ten_transportbackend_t *backend) {
  ten_transportbackend_raw_t *self = (ten_transportbackend_raw_t *)backend;

  if (!self) {
    // TEN_LOGD("empty handle, treat as fail");
    return;
  }

  if (ten_atomic_bool_compare_swap(&self->base.is_close, 0, 1)) {
    // TEN_LOGD("Try to close transport (%s)",
    // ten_string_get_raw_str(self->base.name));

    ten_transport_t *transport = self->base.transport;
    assert(transport);
    process_delayed_tasks(self);
    ten_runloop_async_close(self->delayed_task_signal,
                            on_delayed_task_signal_closed);
  }
}

static void on_delayed_task(ten_runloop_async_t *handle) {
  if (!handle || !handle->data) {
    return;
  }

  ten_transportbackend_raw_t *self = handle->data;
  process_delayed_tasks(self);
}

static ten_transportbackend_t *ten_transportbackend_raw_create(
    ten_transport_t *transport, const ten_string_t *name) {
  ten_transportbackend_raw_t *self = NULL;

  ten_thread_once(&g_init_once, ten_init_stream_raw);

  if (!name || !name->buf || !*name->buf) {
    // TEN_LOGE("Empty name, treat as fail");
    goto error;
  }

  self =
      (ten_transportbackend_raw_t *)malloc(sizeof(ten_transportbackend_raw_t));
  if (!self) {
    // TEN_LOGE("Not enough memory");
    goto error;
  }

  memset(self, 0, sizeof(ten_transportbackend_raw_t));

  ten_transportbackend_init(&self->base, transport, name);
  self->base.connect = ten_transportbackend_raw_connect;
  self->base.listen = ten_transportbackend_raw_listen;
  self->base.close = ten_transportbackend_raw_close;
  ten_list_init(&self->delayed_tasks);
  self->delayed_task_signal = ten_runloop_async_create(transport->loop->impl);
  self->delayed_task_signal->data = self;
  ten_runloop_async_init(self->delayed_task_signal, transport->loop,
                         on_delayed_task);

  return (ten_transportbackend_t *)self;

error:
  ten_transportbackend_raw_close(&self->base);
  return NULL;
};

const ten_transportbackend_factory_t general_tp_backend_raw = {
    .create = ten_transportbackend_raw_create};
