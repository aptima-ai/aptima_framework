//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_utils/io/runloop.h"

#include <stdlib.h>
#include <string.h>
#include <uv.h>

#include "include_internal/axis_utils/io/general/transport/backend/uv/stream/migrate.h"
#include "axis_utils/container/list.h"
#include "axis_utils/io/general/loops/runloop.h"
#include "axis_utils/lib/alloc.h"
#include "axis_utils/lib/atomic.h"
#include "axis_utils/lib/mutex.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/macro/field.h"
#include "axis_utils/macro/mark.h"
#include "axis_utils/macro/memory.h"
#include "axis_utils/sanitizer/thread_check.h"

// Runloop creates its own 'async' in the below 'migrate'.
typedef struct axis_migrate_task_t {
  axis_migrate_t *migrate;
} axis_migrate_task_t;

typedef struct axis_runloop_uv_t {
  axis_runloop_common_t common;

  uv_loop_t *uv_loop;

  // Protect the following 'migrate_tasks'.
  axis_mutex_t *migrate_task_lock;
  // The type of items is the above 'axis_migrate_task_t'.
  axis_list_t migrate_tasks;

  // Start to create 'async' in 'axis_migrate_task_t' in the 'destination'
  // thread/runloop.
  uv_async_t migrate_start_async;
} axis_runloop_uv_t;

typedef struct axis_runloop_async_uv_t {
  axis_runloop_async_common_t common;
  uv_async_t uv_async;
  void (*notify_callback)(axis_runloop_async_t *);
  void (*close_callback)(axis_runloop_async_t *);
} axis_runloop_async_uv_t;

typedef struct axis_runloop_timer_uv_t {
  axis_runloop_timer_common_t common;
  uv_timer_t uv_timer;
  bool initted;
  void (*notify_callback)(axis_runloop_timer_t *, void *);
  void (*stop_callback)(axis_runloop_timer_t *, void *);
  void (*close_callback)(axis_runloop_timer_t *, void *);
} axis_runloop_timer_uv_t;

static void axis_runloop_uv_destroy(axis_runloop_t *loop);
static void axis_runloop_uv_run(axis_runloop_t *loop);
static void *axis_runloop_uv_get_raw(axis_runloop_t *loop);
static void axis_runloop_uv_close(axis_runloop_t *loop);
static void axis_runloop_uv_stop(axis_runloop_t *loop);
static int axis_runloop_async_uv_init(axis_runloop_async_t *base,
                                     axis_runloop_t *loop,
                                     void (*callback)(axis_runloop_async_t *));
static void axis_runloop_async_uv_close(axis_runloop_async_t *base,
                                       void (*close_cb)(axis_runloop_async_t *));
static int axis_runloop_async_uv_notify(axis_runloop_async_t *base);
static void axis_runloop_async_uv_destroy(axis_runloop_async_t *base);
static int axis_runloop_uv_alive(axis_runloop_t *loop);

// Start to create 'async' in 'axis_migrate_task_t'.
static void migrate_start_async_callback(uv_async_t *migrate_start_async) {
  axis_ASSERT(migrate_start_async, "Invalid argument.");

  axis_runloop_uv_t *to_loop_impl = migrate_start_async->data;
  axis_ASSERT(to_loop_impl &&
                 axis_runloop_check_integrity(&to_loop_impl->common.base, true),
             "Invalid argument.");

  axis_list_t tasks = axis_LIST_INIT_VAL;

  {
    // Get all migration tasks at once.

    axis_UNUSED int rc = axis_mutex_lock(to_loop_impl->migrate_task_lock);
    axis_ASSERT(!rc, "Failed to lock.");

    axis_list_swap(&to_loop_impl->migrate_tasks, &tasks);

    rc = axis_mutex_unlock(to_loop_impl->migrate_task_lock);
    axis_ASSERT(!rc, "Failed to unlock.");
  }

  // Handle each migration task one by one.
  axis_list_foreach (&tasks, iter) {
    axis_migrate_task_t *task = axis_ptr_listnode_get(iter.node);
    axis_ASSERT(task, "Invalid argument.");

    {
      // Initialize the sufficient 'async' resources in the migration task, and
      // all the 'dst_xxx' 'async' resources should belong to the 'to'
      // thread/runloop.

      // Ensure the 'async' is created in the correct (belonging) thread.
      axis_runloop_t *to_runloop = task->migrate->to;
      axis_ASSERT(to_runloop && axis_runloop_check_integrity(to_runloop, true),
                 "Invalid argument.");

      axis_UNUSED int rc =
          uv_async_init(migrate_start_async->loop, &task->migrate->dst_prepare,
                        migration_dst_prepare);
      axis_ASSERT(!rc, "uv_async_init() failed: %d", rc);

      rc = uv_async_init(migrate_start_async->loop,
                         &task->migrate->dst_migration, migration_dst_start);
      axis_ASSERT(!rc, "uv_async_init() failed: %d", rc);
    }

    axis_stream_migrate_uv_stage2(task->migrate);
  }

  axis_list_clear(&tasks);
}

static void free_task(void *task) { axis_FREE(task); }

void axis_migrate_task_create_and_insert(axis_migrate_t *migrate) {
  axis_ASSERT(migrate, "Invalid argument.");

  axis_runloop_uv_t *to_runloop = (axis_runloop_uv_t *)(migrate->to);
  axis_ASSERT(to_runloop &&
                 // axis_NOLINTNEXTLINE(thread-check)
                 // thread-check: This function is intended to be called outside
                 // of 'to_runloop' thread.
                 axis_runloop_check_integrity(&to_runloop->common.base, false),
             "Invalid argument.");

  axis_migrate_task_t *task =
      (axis_migrate_task_t *)axis_MALLOC(sizeof(axis_migrate_task_t));
  axis_ASSERT(task, "Failed to allocate memory.");

  task->migrate = migrate;

  {
    axis_UNUSED int rc = axis_mutex_lock(to_runloop->migrate_task_lock);
    axis_ASSERT(!rc, "Failed to lock.");

    axis_list_push_ptr_back(&to_runloop->migrate_tasks, task, free_task);

    rc = axis_mutex_unlock(to_runloop->migrate_task_lock);
    axis_ASSERT(!rc, "Failed to unlock.");
  }

  // Kick 'to_runloop', so that the later operations are in the 'to'
  // thread.
  axis_UNUSED int rc = uv_async_send(&to_runloop->migrate_start_async);
  axis_ASSERT(!rc, "uv_async_send() failed: %d", rc);
}

/**
 * @brief Create sufficient resources to migrate 'stream' from one
 * thread/runloop to another. libuv is not a thread-safe library, so a 'stream'
 * must be migrated to the thread where uses it.
 */
static void axis_runloop_create_uv_migration_resource(axis_runloop_uv_t *impl) {
  axis_ASSERT(impl, "Invalid argument.");

  impl->migrate_task_lock = axis_mutex_create();
  axis_ASSERT(impl->migrate_task_lock, "Should not happen.");

  axis_list_init(&impl->migrate_tasks);

  axis_UNUSED int rc = uv_async_init(impl->uv_loop, &impl->migrate_start_async,
                                    migrate_start_async_callback);
  axis_ASSERT(!rc, "uv_async_init() failed: %d", rc);

  impl->migrate_start_async.data = impl;
}

static axis_runloop_common_t *axis_runloop_create_uv_common(void *raw) {
  axis_runloop_uv_t *impl =
      (axis_runloop_uv_t *)axis_MALLOC(sizeof(axis_runloop_uv_t));
  axis_ASSERT(impl, "Failed to allocate memory.");
  if (!impl) {
    return NULL;
  }

  memset(impl, 0, sizeof(axis_runloop_uv_t));

  impl->common.base.impl = axis_strdup(axis_RUNLOOP_UV);
  if (raw) {
    impl->uv_loop = raw;
  } else {
    impl->uv_loop = axis_MALLOC(sizeof(uv_loop_t));
    axis_ASSERT(impl->uv_loop, "Failed to allocate memory.");

    axis_UNUSED int rc = uv_loop_init(impl->uv_loop);
    axis_ASSERT(!rc, "uv_loop_init() failed: %d", rc);
  }

  axis_runloop_create_uv_migration_resource(impl);

  impl->common.destroy = axis_runloop_uv_destroy;
  impl->common.run = axis_runloop_uv_run;
  impl->common.get_raw = axis_runloop_uv_get_raw;
  impl->common.stop = axis_runloop_uv_stop;
  impl->common.alive = axis_runloop_uv_alive;

  return &impl->common;
}

axis_runloop_common_t *axis_runloop_create_uv(void) {
  return axis_runloop_create_uv_common(NULL);
}

axis_runloop_common_t *axis_runloop_attach_uv(void *raw) {
  return axis_runloop_create_uv_common(raw);
}

axis_runloop_async_common_t *axis_runloop_async_create_uv(void) {
  axis_runloop_async_uv_t *impl =
      (axis_runloop_async_uv_t *)axis_MALLOC(sizeof(axis_runloop_async_uv_t));
  axis_ASSERT(impl, "Failed to allocate memory.");
  if (!impl) {
    return NULL;
  }

  memset(impl, 0, sizeof(axis_runloop_async_uv_t));

  impl->common.base.impl = axis_strdup(axis_RUNLOOP_UV);
  impl->common.init = axis_runloop_async_uv_init;
  impl->common.close = axis_runloop_async_uv_close;
  impl->common.destroy = axis_runloop_async_uv_destroy;
  impl->common.notify = axis_runloop_async_uv_notify;

  return &impl->common;
}

static void axis_runloop_uv_destroy(axis_runloop_t *loop) {
  axis_runloop_uv_t *impl = (axis_runloop_uv_t *)loop;
  axis_ASSERT(impl &&
                 // axis_NOLINTNEXTLINE(thread-check)
                 // thread-check: This function is intended to be called in any
                 // threads.
                 axis_runloop_check_integrity(loop, false),
             "Invalid argument.");

  if (!loop || strcmp(loop->impl, axis_RUNLOOP_UV) != 0) {
    return;
  }

  axis_sanitizer_thread_check_deinit(&loop->thread_check);

  axis_FREE(impl->common.base.impl);
  axis_FREE(impl);
}

static void axis_runloop_uv_run(axis_runloop_t *loop) {
  axis_runloop_uv_t *impl = (axis_runloop_uv_t *)loop;
  axis_ASSERT(impl && axis_runloop_check_integrity(loop, true),
             "Invalid argument.");

  if (!loop || strcmp(loop->impl, axis_RUNLOOP_UV) != 0) {
    return;
  }

  uv_run(impl->uv_loop, UV_RUN_DEFAULT);

  // Must call 'uv_loop_close()' after 'uv_run()' to release the internal libuv
  // loop resources, and check the return value of 'uv_loop_close' to ensure
  // that all resources relevant to the runloop is released.
  axis_UNUSED int rc = uv_loop_close(impl->uv_loop);
  axis_ASSERT(!rc, "Runloop is destroyed when it holds alive resources.");

  axis_FREE(impl->uv_loop);

  // The runloop is stopped completely, call the on_stopped callback if the
  // user registered one before.
  if (impl->common.on_stopped) {
    impl->common.on_stopped((axis_runloop_t *)impl,
                            impl->common.on_stopped_data);
  }
}

static void *axis_runloop_uv_get_raw(axis_runloop_t *loop) {
  axis_runloop_uv_t *impl = (axis_runloop_uv_t *)loop;
  axis_ASSERT(impl &&
                 // axis_NOLINTNEXTLINE(thread-check)
                 // thread-check: Refer to axis_runloop_get_raw(), this function
                 // is intended to be called in any threads.
                 axis_runloop_check_integrity(loop, false),
             "Invalid argument.");

  if (!loop || strcmp(loop->impl, axis_RUNLOOP_UV) != 0) {
    return NULL;
  }

  return impl->uv_loop;
}

static void axis_runloop_uv_migration_start_async_closed(uv_handle_t *handle) {
  axis_ASSERT(handle, "Invalid argument.");

  axis_runloop_uv_t *impl = (axis_runloop_uv_t *)handle->data;
  axis_ASSERT(impl && axis_runloop_check_integrity(&impl->common.base, true),
             "Invalid argument.");

  if (!axis_atomic_load(&impl->common.attach_other)) {
    // If the underlying runloop is created separately, and be wrapped into the
    // ten runloop, then we should _not_ stop the underlying runloop.
    uv_stop(impl->uv_loop);
  } else {
    // Otherwise, the runloop is stopped completely, call the on_stopped
    // callback if the user registered one before.
    if (impl->common.on_stopped) {
      impl->common.on_stopped((axis_runloop_t *)impl,
                              impl->common.on_stopped_data);
    }
  }

  axis_mutex_destroy(impl->migrate_task_lock);
  axis_list_clear(&impl->migrate_tasks);
}

static void axis_runloop_uv_stop(axis_runloop_t *loop) {
  axis_runloop_uv_t *impl = (axis_runloop_uv_t *)loop;
  axis_ASSERT(impl && axis_runloop_check_integrity(loop, true),
             "Invalid argument.");

  if (!loop || strcmp(loop->impl, axis_RUNLOOP_UV) != 0) {
    return;
  }

  // Close migration relevant resources.
  uv_close((uv_handle_t *)&impl->migrate_start_async,
           axis_runloop_uv_migration_start_async_closed);
}

static int axis_runloop_uv_alive(axis_runloop_t *loop) {
  axis_runloop_uv_t *impl = (axis_runloop_uv_t *)loop;
  axis_ASSERT(impl && axis_runloop_check_integrity(loop, true),
             "Invalid argument.");

  if (!loop || strcmp(loop->impl, axis_RUNLOOP_UV) != 0) {
    return 0;
  }

  return uv_loop_alive(impl->uv_loop);
}

static void uv_async_callback(uv_async_t *async) {
  axis_runloop_async_uv_t *async_impl =
      (axis_runloop_async_uv_t *)CONTAINER_OF_FROM_FIELD(
          async, axis_runloop_async_uv_t, uv_async);

  if (!async) {
    return;
  }

  if (async_impl->notify_callback) {
    async_impl->notify_callback(&async_impl->common.base);
  }
}

static int axis_runloop_async_uv_init(
    axis_runloop_async_t *base, axis_runloop_t *loop,
    void (*notify_callback)(axis_runloop_async_t *)) {
  axis_runloop_async_uv_t *async_impl = (axis_runloop_async_uv_t *)base;
  axis_runloop_uv_t *loop_impl = (axis_runloop_uv_t *)loop;

  if (!base || strcmp(base->impl, axis_RUNLOOP_UV) != 0) {
    return -1;
  }

  if (!loop || strcmp(loop->impl, axis_RUNLOOP_UV) != 0) {
    return -1;
  }

  axis_ASSERT(base && axis_runloop_async_check_integrity(base, true),
             "Invalid argument.");
  axis_ASSERT(loop && axis_runloop_check_integrity(loop, true),
             "Invalid argument.");

  async_impl->notify_callback = notify_callback;
  int rc = uv_async_init(loop_impl->uv_loop, &async_impl->uv_async,
                         uv_async_callback);
  axis_ASSERT(!rc, "uv_async_init() failed: %d", rc);

  return rc;
}

static void uv_async_closed(uv_handle_t *handle) {
  axis_runloop_async_uv_t *async_impl =
      (axis_runloop_async_uv_t *)CONTAINER_OF_FROM_FIELD(
          handle, axis_runloop_async_uv_t, uv_async);
  if (!handle) {
    return;
  }

  if (!async_impl->close_callback) {
    return;
  }

  async_impl->common.base.loop = NULL;
  async_impl->close_callback(&async_impl->common.base);
}

static void axis_runloop_async_uv_close(
    axis_runloop_async_t *base, void (*close_cb)(axis_runloop_async_t *)) {
  axis_runloop_async_uv_t *async_impl = (axis_runloop_async_uv_t *)base;

  if (!base || strcmp(base->impl, axis_RUNLOOP_UV) != 0) {
    return;
  }

  axis_ASSERT(base && axis_runloop_async_check_integrity(base, true),
             "Invalid argument.");

  async_impl->close_callback = close_cb;
  uv_close((uv_handle_t *)&async_impl->uv_async, uv_async_closed);
}

static void axis_runloop_async_uv_destroy(axis_runloop_async_t *base) {
  axis_runloop_async_uv_t *async_impl = (axis_runloop_async_uv_t *)base;

  if (!base || strcmp(base->impl, axis_RUNLOOP_UV) != 0) {
    return;
  }

  axis_ASSERT(base && axis_runloop_async_check_integrity(base, true),
             "Invalid argument.");

  axis_sanitizer_thread_check_deinit(&base->thread_check);

  axis_FREE(async_impl->common.base.impl);
  axis_FREE(async_impl);
}

static int axis_runloop_async_uv_notify(axis_runloop_async_t *base) {
  axis_runloop_async_uv_t *async_impl = (axis_runloop_async_uv_t *)base;

  if (!base || strcmp(base->impl, axis_RUNLOOP_UV) != 0) {
    return -1;
  }

  axis_ASSERT(base &&
                 // axis_NOLINTNEXTLINE(thread-check)
                 // thread-check: This function is intended to be called in any
                 // threads.
                 axis_runloop_async_check_integrity(base, false),
             "Invalid argument.");

  int rc = uv_async_send(&async_impl->uv_async);
  axis_ASSERT(!rc, "uv_async_send() failed: %d", rc);

  return rc;
}

static void uv_timer_callback(uv_timer_t *handle) {
  axis_runloop_timer_uv_t *timer_impl =
      (axis_runloop_timer_uv_t *)CONTAINER_OF_FROM_FIELD(
          handle, axis_runloop_timer_uv_t, uv_timer);

  if (!handle) {
    return;
  }

  if (timer_impl->notify_callback) {
    timer_impl->notify_callback(&timer_impl->common.base,
                                timer_impl->common.start_data);
  }
}

static int axis_runloop_timer_uv_start(
    axis_runloop_timer_t *base, axis_runloop_t *loop,
    void (*notify_callback)(axis_runloop_timer_t *, void *)) {
  axis_runloop_timer_uv_t *timer_impl = (axis_runloop_timer_uv_t *)base;
  axis_runloop_uv_t *loop_impl = (axis_runloop_uv_t *)loop;

  if (!base || strcmp(base->impl, axis_RUNLOOP_UV) != 0) {
    return -1;
  }

  if (!loop || strcmp(loop->impl, axis_RUNLOOP_UV) != 0) {
    return -1;
  }

  axis_ASSERT(axis_runloop_timer_check_integrity(base, true),
             "Invalid argument.");
  axis_ASSERT(loop && axis_runloop_check_integrity(loop, true),
             "Invalid argument.");

  timer_impl->notify_callback = notify_callback;
  if (timer_impl->initted == false) {
    int rc = uv_timer_init(loop_impl->uv_loop, &timer_impl->uv_timer);
    if (rc != 0) {
      axis_ASSERT(!rc, "uv_timer_init() failed: %d", rc);
      return -1;
    }
    timer_impl->initted = true;
  }

  int rc = uv_timer_start(&timer_impl->uv_timer, uv_timer_callback,
                          base->timeout, base->periodic);
  axis_ASSERT(!rc, "uv_timer_start() failed: %d", rc);

  return rc;
}

static void uv_timer_closed(uv_handle_t *handle) {
  axis_runloop_timer_uv_t *timer_impl =
      (axis_runloop_timer_uv_t *)CONTAINER_OF_FROM_FIELD(
          handle, axis_runloop_timer_uv_t, uv_timer);
  if (!handle) {
    return;
  }

  if (!timer_impl->close_callback) {
    return;
  }

  timer_impl->common.base.loop = NULL;
  if (timer_impl->close_callback) {
    timer_impl->close_callback(&timer_impl->common.base,
                               timer_impl->common.close_data);
  }
}

static void axis_runloop_timer_uv_stop(axis_runloop_timer_t *base,
                                      void (*stop_cb)(axis_runloop_timer_t *,
                                                      void *)) {
  axis_runloop_timer_uv_t *timer_impl = (axis_runloop_timer_uv_t *)base;

  if (!base || strcmp(base->impl, axis_RUNLOOP_UV) != 0) {
    return;
  }

  axis_ASSERT(axis_runloop_timer_check_integrity(base, true),
             "Invalid argument.");

  timer_impl->stop_callback = stop_cb;

  axis_UNUSED int rc = uv_timer_stop(&timer_impl->uv_timer);
  axis_ASSERT(!rc, "uv_timer_stop() failed: %d", rc);

  if (timer_impl->stop_callback) {
    timer_impl->stop_callback(&timer_impl->common.base,
                              timer_impl->common.stop_data);
  }
}

static void axis_runloop_timer_uv_close(axis_runloop_timer_t *base,
                                       void (*close_cb)(axis_runloop_timer_t *,
                                                        void *)) {
  axis_runloop_timer_uv_t *timer_impl = (axis_runloop_timer_uv_t *)base;

  if (!base || strcmp(base->impl, axis_RUNLOOP_UV) != 0) {
    return;
  }

  axis_ASSERT(axis_runloop_timer_check_integrity(base, true),
             "Invalid argument.");

  timer_impl->close_callback = close_cb;
  uv_close((uv_handle_t *)&timer_impl->uv_timer, uv_timer_closed);
}

static void axis_runloop_timer_uv_destroy(axis_runloop_timer_t *base) {
  axis_runloop_timer_uv_t *timer_impl = (axis_runloop_timer_uv_t *)base;

  if (!base || strcmp(base->impl, axis_RUNLOOP_UV) != 0) {
    return;
  }

  axis_ASSERT(axis_runloop_timer_check_integrity(base, true),
             "Invalid argument.");

  axis_sanitizer_thread_check_deinit(&base->thread_check);

  axis_FREE(timer_impl->common.base.impl);
  axis_FREE(timer_impl);
}

axis_runloop_timer_common_t *axis_runloop_timer_create_uv(void) {
  axis_runloop_timer_uv_t *impl =
      (axis_runloop_timer_uv_t *)axis_MALLOC(sizeof(axis_runloop_timer_uv_t));
  axis_ASSERT(impl, "Failed to allocate memory.");
  if (!impl) {
    return NULL;
  }

  memset(impl, 0, sizeof(axis_runloop_timer_uv_t));

  impl->initted = false;
  impl->common.base.impl = axis_strdup(axis_RUNLOOP_UV);
  impl->common.start = axis_runloop_timer_uv_start;
  impl->common.stop = axis_runloop_timer_uv_stop;
  impl->common.close = axis_runloop_timer_uv_close;
  impl->common.destroy = axis_runloop_timer_uv_destroy;

  return &impl->common;
}
