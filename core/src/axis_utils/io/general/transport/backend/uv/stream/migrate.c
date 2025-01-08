//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_utils/io/general/transport/backend/uv/stream/migrate.h"

#include <stdlib.h>

#include "include_internal/axis_utils/io/general/transport/backend/uv/stream/tcp.h"
#include "include_internal/axis_utils/io/runloop.h"
#include "axis_utils/io/general/loops/uv/runloop.h"
#include "axis_utils/io/runloop.h"
#include "axis_utils/lib/alloc.h"
#include "axis_utils/lib/atomic.h"
#include "axis_utils/log/log.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/macro/mark.h"

#if defined(_WIN32)
#include "axis_utils/lib/task.h"
#endif

static void migration_dst_done(uv_stream_t *pipe_, ssize_t nread,
                               const uv_buf_t *buf);
static void migration_src_pipe_closed(uv_handle_t *pipe);
static void migration_dst_pipe_closed(uv_handle_t *pipe);
static void migration_finalize_callback(uv_handle_t *handle);

static void simple_close_callback(uv_handle_t *handle) { axis_FREE(handle); }

static void free_write_req_after_migration(uv_write_t *req) { axis_FREE(req); }

static void alloc_buf_for_pipe_data(axis_UNUSED uv_handle_t *handle,
                                    size_t suggested_size, uv_buf_t *buf) {
  buf->base = axis_MALLOC(suggested_size);
  axis_ASSERT(buf->base, "Failed to allocate memory.");

  buf->len = suggested_size;
}

static void migration_src_prepare(uv_async_t *async) {
  axis_migrate_t *migrate = (axis_migrate_t *)async->data;

  // Initialize pipe with 'ipc == 1', so that we can transfer the socket file
  // descriptor.
  uv_pipe_init(axis_runloop_get_raw(migrate->from), migrate->pipe[0],
               1 /* ipc */);
  migrate->pipe[0]->data = migrate;

  // Bind one socket fd to one end of the pipe.
  uv_pipe_open(migrate->pipe[0], migrate->fds[0]);
#if defined(_WIN32)
  // |ipc_remote_pid| will set to _parent_id_ in Windows which make no sense at
  // all in single process mode. In multiple process mode this assumption also
  // fails at most of the time Currently we ignore "multi-process" things of uv
  // in Windows
  migrate->pipe[0]->pipe.conn.ipc_remote_pid = axis_task_get_id();
#endif

  uv_async_send(&migrate->dst_prepare);
}

void migration_dst_prepare(uv_async_t *async) {
  axis_ASSERT(async, "Invalid argument.");

  axis_migrate_t *migrate = (axis_migrate_t *)async->data;
  axis_ASSERT(migrate, "Invalid argument.");

  uv_pipe_init(axis_runloop_get_raw(migrate->to), migrate->pipe[1], 1 /* ipc */);
  uv_pipe_open(migrate->pipe[1], migrate->fds[1]);
  migrate->pipe[1]->data = migrate;
  uv_async_send(&migrate->src_migration);
}

static void migration_src_start(uv_async_t *async) {
  axis_migrate_t *migrate = (axis_migrate_t *)async->data;

  axis_streambackend_tcp_t *tcp_stream =
      (axis_streambackend_tcp_t *)migrate->stream->backend;

  uv_write_t *write_req = (uv_write_t *)axis_MALLOC(sizeof(uv_write_t));
  axis_ASSERT(write_req, "Failed to allocate memory.");

  uv_buf_t buf = uv_buf_init(".", 1);
  uv_write2(write_req, (uv_stream_t *)migrate->pipe[0], &buf, 1,
            tcp_stream->uv_stream, free_write_req_after_migration);

  uv_async_send(&migrate->dst_migration);
}

void migration_dst_start(uv_async_t *async) {
  axis_migrate_t *migrate = (axis_migrate_t *)async->data;
  uv_read_start((uv_stream_t *)migrate->pipe[1], alloc_buf_for_pipe_data,
                migration_dst_done);
}

static void migration_dst_done(uv_stream_t *pipe_, ssize_t nread,
                               const uv_buf_t *buf) {
  axis_migrate_t *migrate = (axis_migrate_t *)pipe_->data;
  uv_pipe_t *pipe = (uv_pipe_t *)pipe_;

  if (nread < 0) {
    // axis_LOGE("Migration fail, read error: %s", uv_strerror(nread));
  }

  if (migrate->migrate_processed) {
    // axis_LOGE("Migration fail because already processed");
    return;
  }

  migrate->migrate_processed = 1;

  uv_read_stop(pipe);

  // Create a new stream which bound to the eventloop of the engine.
  axis_stream_t *stream =
      axis_stream_tcp_create_uv(axis_runloop_get_raw(migrate->to));
  axis_streambackend_tcp_t *tcp_stream =
      (axis_streambackend_tcp_t *)stream->backend;

  // Accepting to bind the fd of the physical channel to the new 'stream'.
  if (uv_accept((uv_stream_t *)pipe, tcp_stream->uv_stream) == 0) {
    if (migrate->migrated) {
      migrate->migrated(stream, migrate->user_data);
    }
  } else {
    // axis_LOGE("Migration fail");
    axis_ASSERT(0, "Invalid argument.");
  }

  if (buf && buf->base) {
    axis_FREE_(buf->base);
  }

  axis_atomic_store(&migrate->expect_finalize_count, 6);
  axis_atomic_store(&migrate->finalized_count, 0);
  uv_close((uv_handle_t *)migrate->pipe[0], migration_finalize_callback);
  uv_close((uv_handle_t *)migrate->pipe[1], migration_finalize_callback);
  uv_close(&migrate->src_prepare, migration_finalize_callback);
  uv_close(&migrate->dst_prepare, migration_finalize_callback);
  uv_close(&migrate->src_migration, migration_finalize_callback);
  uv_close(&migrate->dst_migration, migration_finalize_callback);
}

static void migration_finalize_callback(uv_handle_t *handle) {
  axis_migrate_t *migrate = (axis_migrate_t *)handle->data;

  if (axis_atomic_add_fetch(&migrate->finalized_count, 1) ==
      axis_atomic_load(&migrate->expect_finalize_count)) {
    axis_FREE(migrate->pipe[0]);
    axis_FREE(migrate->pipe[1]);
    axis_FREE(migrate);
  }
}

int axis_stream_migrate_uv_stage2(axis_migrate_t *migrate) {
  axis_ASSERT(migrate, "Invalid argument.");

  migrate->src_prepare.data = migrate;
  migrate->src_migration.data = migrate;
  migrate->dst_prepare.data = migrate;
  migrate->dst_migration.data = migrate;

#if !defined(_WIN32)
  axis_UNUSED int rc = uv_socketpair(SOCK_STREAM, 0, migrate->fds, 0, 0);
#else
  int pipe_flags = UV_NONBLOCK_PIPE | UV_READABLE_PIPE | UV_WRITABLE_PIPE;
  axis_UNUSED int rc = uv_pipe(migrate->fds, pipe_flags, pipe_flags);
#endif
  axis_ASSERT(!rc, "Invalid argument.");

  migrate->pipe[0] = (uv_pipe_t *)axis_MALLOC(sizeof(uv_pipe_t));
  axis_ASSERT(migrate->pipe[0], "Failed to allocate memory.");

  migrate->pipe[1] = (uv_pipe_t *)axis_MALLOC(sizeof(uv_pipe_t));
  axis_ASSERT(migrate->pipe[1], "Failed to allocate memory.");

  migrate->pipe[0]->data = migrate;
  migrate->pipe[1]->data = migrate;

  // Kick 'from' runloop, so that the later operations happen in the 'from'
  // thread.
  uv_async_send(&migrate->src_prepare);

  return 0;
}

// This function happens in the 'from' thread.
static int axis_stream_migrate_uv_stage1(axis_stream_t *self, axis_runloop_t *from,
                                        axis_runloop_t *to, void **user_data,
                                        void (*cb)(axis_stream_t *new_stream,
                                                   void **user_data)) {
  axis_ASSERT(from && axis_runloop_check_integrity(from, true),
             "Invalid argument.");
  axis_ASSERT(to &&
                 // axis_NOLINTNEXTLINE(thread-check)
                 // thread-check: Stage 1 happens in the 'from' thread.
                 axis_runloop_check_integrity(from, false),
             "Invalid argument.");

  axis_migrate_t *migrate = NULL;
  uv_loop_t *from_loop = NULL;
  uv_loop_t *to_loop = NULL;

  from_loop = axis_runloop_get_raw(from);
  to_loop = axis_runloop_get_raw(to);
  if (!from_loop || !to_loop) {
    axis_LOGE("Invalid parameter, from loop %p, to loop %p", from_loop, to_loop);
    return -1;
  }

  if (!uv_loop_alive(from_loop)) {
    axis_LOGE("Source loop not alive");
    return -1;
  }

  migrate = (axis_migrate_t *)axis_MALLOC(sizeof(axis_migrate_t));
  axis_ASSERT(migrate, "Failed to allocate memory.");

  memset(migrate, 0, sizeof(axis_migrate_t));
  migrate->stream = self;
  migrate->from = from;
  migrate->to = to;
  migrate->user_data = user_data;
  migrate->migrated = cb;

  // Initialize the 'src_async' in the 'from' thread.
  uv_async_init(from_loop, &migrate->src_prepare, migration_src_prepare);
  uv_async_init(from_loop, &migrate->src_migration, migration_src_start);

  axis_migrate_task_create_and_insert(migrate);

  return 0;
}

int axis_stream_migrate_uv(axis_stream_t *self, axis_runloop_t *from,
                          axis_runloop_t *to, void **user_data,
                          void (*cb)(axis_stream_t *new_stream,
                                     void **user_data)) {
  return axis_stream_migrate_uv_stage1(self, from, to, user_data, cb);
}
