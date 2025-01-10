//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_utils/io/runloop.h"

#include <stdint.h>
#include <stdlib.h>

#include "axis_utils/container/list.h"
#include "axis_utils/io/general/loops/runloop.h"
#include "axis_utils/lib/alloc.h"
#include "axis_utils/lib/atomic.h"
#include "axis_utils/lib/mutex.h"
#include "axis_utils/lib/thread_local.h"
#include "axis_utils/lib/thread_once.h"
#include "axis_utils/log/log.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/macro/field.h"
#include "axis_utils/macro/mark.h"

static axis_thread_once_t runloop_once = axis_THREAD_ONCE_INIT;
static axis_thread_key_t runloop_key = kInvalidTlsKey;

static void setup_runloop_callback(void) {
  runloop_key = axis_thread_key_create();
}

static int set_self(axis_runloop_t *self) {
  axis_thread_key_t key = runloop_key;

  if (key == kInvalidTlsKey) {
    axis_LOGE("Failed to set the runloop pointer to the thread local storage.");
    return -1;
  }

  return axis_thread_set_key(key, self);
}

static axis_runloop_t *get_self(void) {
  axis_thread_key_t key = runloop_key;

  if (key == kInvalidTlsKey) {
    axis_LOGE(
        "Failed to get the runloop pointer from the thread local storage.");
    return NULL;
  }

  return axis_thread_get_key(key);
}

#if defined(axis_USE_LIBUV)
extern axis_runloop_common_t *axis_runloop_create_uv(void);
extern axis_runloop_common_t *axis_runloop_attach_uv(void *);
extern axis_runloop_async_common_t *axis_runloop_async_create_uv(void);
extern axis_runloop_timer_common_t *axis_runloop_timer_create_uv(void);
#endif

#if defined(axis_USE_LIBEVENT)
extern axis_runloop_common_t *axis_runloop_create_event(void);
extern axis_runloop_common_t *axis_runloop_attach_event(void *);
extern axis_runloop_async_common_t *axis_runloop_async_create_event(void);
extern axis_runloop_timer_common_t *axis_runloop_timer_create_event(void);
#endif

#if defined(axis_USE_BARE_RUNLOOP)
extern axis_runloop_common_t *axis_runloop_create_bare(void);
extern axis_runloop_common_t *axis_runloop_attach_bare(void *);
extern axis_runloop_async_common_t *axis_runloop_async_create_bare(void);
extern axis_runloop_timer_common_t *axis_runloop_timer_create_bare(void);
#endif

typedef struct runloop_factory_t {
  const char *impl;
  axis_runloop_common_t *(*create_runloop)(void);
  axis_runloop_common_t *(*attach)(void *raw);
  axis_runloop_async_common_t *(*create_async)(void);
  axis_runloop_timer_common_t *(*create_timer)(void);
} runloop_factory_t;

static const runloop_factory_t runloop_factory[] = {
#if defined(axis_USE_LIBUV)
    // libuv is the default runloop, put it in the first element.
    {
        axis_RUNLOOP_UV,
        axis_runloop_create_uv,
        axis_runloop_attach_uv,
        axis_runloop_async_create_uv,
        axis_runloop_timer_create_uv,
    },
#endif
#if defined(axis_USE_LIBEVENT)
    {
        axis_RUNLOOP_EVENT2,
        axis_runloop_create_event,
        axis_runloop_attach_event,
        axis_runloop_async_create_event,
        axis_runloop_timer_create_event,
    },
#endif
#if defined(axis_USE_BARE_RUNLOOP)
    {
        axis_RUNLOOP_BARE,
        axis_runloop_create_bare,
        axis_runloop_attach_bare,
        axis_runloop_async_create_bare,
        axis_runloop_timer_create_bare,
    },
#endif
    {
        NULL,
        NULL,
        NULL,
        NULL,
    }};

#define RUNLOOP_FACTORY_SIZE \
  (sizeof(runloop_factory) / sizeof(runloop_factory[0]))

static const char *get_default_impl(void) { return runloop_factory[0].impl; }

bool axis_runloop_check_integrity(axis_runloop_t *self, bool check_thread) {
  axis_ASSERT(self, "Should not happen.");

  if (axis_signature_get(&self->signature) !=
      (axis_signature_t)axis_RUNLOOP_SIGNATURE) {
    return false;
  }

  if (check_thread &&
      !axis_sanitizer_thread_check_do_check(&self->thread_check)) {
    return false;
  }

  return true;
}

bool axis_runloop_async_check_integrity(axis_runloop_async_t *self,
                                       bool check_thread) {
  axis_ASSERT(self, "Should not happen.");

  if (axis_signature_get(&self->signature) !=
      (axis_signature_t)axis_RUNLOOP_ASYNC_SIGNATURE) {
    return false;
  }

  if (check_thread &&
      !axis_sanitizer_thread_check_do_check(&self->thread_check)) {
    return false;
  }

  return true;
}

bool axis_runloop_timer_check_integrity(axis_runloop_timer_t *self,
                                       bool check_thread) {
  axis_ASSERT(self, "Should not happen.");

  if (axis_signature_get(&self->signature) !=
      (axis_signature_t)axis_RUNLOOP_TIMER_SIGNATURE) {
    return false;
  }

  if (check_thread &&
      !axis_sanitizer_thread_check_do_check(&self->thread_check)) {
    return false;
  }

  return true;
}

static const runloop_factory_t *get_runloop_factory(const char *name) {
  if (!name) {
    return NULL;
  }

  for (int i = 0; i < RUNLOOP_FACTORY_SIZE; i++) {
    if (!runloop_factory[i].impl) {
      continue;
    }

    if (strcmp(name, runloop_factory[i].impl) != 0) {
      continue;
    }

    return &runloop_factory[i];
  }

  return NULL;
}

static void process_remaining_tasks_safe(axis_runloop_common_t *loop) {
  axis_ASSERT(loop && axis_runloop_check_integrity(&loop->base, true),
             "Invalid argument.");

  while (!axis_list_is_empty(&loop->tasks)) {
    axis_listnode_t *itor = axis_list_pop_front(&loop->tasks);
    axis_ASSERT(itor, "Invalid argument.");

    axis_runloop_task_t *task = (axis_runloop_task_t *)CONTAINER_OF_FROM_FIELD(
        itor, axis_runloop_task_t, node);

    if (task->func) {
      axis_UNUSED bool rc = axis_mutex_unlock(loop->lock);
      axis_ASSERT(!rc, "Failed to unlock.");

      task->func(task->from, task->arg);

      rc = axis_mutex_lock(loop->lock);
      axis_ASSERT(!rc, "Failed to lock.");
    }

    axis_FREE(task);
  }
}

static void flush_remaining_tasks(axis_runloop_common_t *impl) {
  axis_ASSERT(impl && axis_runloop_check_integrity(&impl->base, true),
             "Invalid argument.");

  axis_UNUSED bool rc = axis_mutex_lock(impl->lock);
  axis_ASSERT(!rc, "Failed to lock.");

  process_remaining_tasks_safe(impl);

  rc = axis_mutex_unlock(impl->lock);
  axis_ASSERT(!rc, "Failed to unlock.");
}

static void task_available_callback(axis_runloop_async_t *async) {
  axis_ASSERT(async && axis_runloop_async_check_integrity(async, true),
             "Invalid argument.");

  axis_runloop_common_t *impl = (axis_runloop_common_t *)async->data;
  axis_ASSERT(impl && axis_runloop_check_integrity(&impl->base, true),
             "Invalid argument.");

  flush_remaining_tasks(impl);
}

static void task_available_signal_closed(axis_runloop_async_t *async) {
  axis_ASSERT(async && axis_runloop_async_check_integrity(async, true),
             "Invalid argument.");

  axis_runloop_common_t *impl = (axis_runloop_common_t *)async->data;
  axis_ASSERT(impl && axis_runloop_check_integrity(&impl->base, true),
             "Invalid argument.");

  // After the 'signal' is closed, we can ensure that there will be no more
  // new tasks be added to the task queue, so we can safely comsume all the
  // remaining tasks here.
  task_available_callback(async);

  // All the remaining tasks should be done.
  axis_ASSERT(axis_list_is_empty(&impl->tasks), "Should not happen.");

  if (impl->stop) {
    impl->stop(&impl->base);
  }

  axis_runloop_async_destroy(async);

  impl->task_available_signal = NULL;
}

static void runloop_init(axis_runloop_common_t *impl, int64_t attached) {
  axis_ASSERT(impl, "Invalid argument.");

  axis_signature_set(&impl->base.signature, axis_RUNLOOP_SIGNATURE);
  axis_sanitizer_thread_check_init_with_current_thread(&impl->base.thread_check);

  axis_atomic_store((axis_atomic_t *)&impl->state, axis_RUNLOOP_STATE_IDLE);
  axis_atomic_store(&impl->attach_other, attached);
  axis_list_init(&impl->tasks);
  impl->lock = axis_mutex_create();
  impl->task_available_signal = axis_runloop_async_create(impl->base.impl);
  impl->task_available_signal->data = impl;
  axis_runloop_async_init(impl->task_available_signal, &impl->base,
                         task_available_callback);
}

axis_runloop_t *axis_runloop_create(const char *type) {
  const char *name = type ? type : get_default_impl();
  axis_runloop_common_t *impl = NULL;
  const runloop_factory_t *factory = NULL;

  axis_thread_once(&runloop_once, setup_runloop_callback);

  factory = get_runloop_factory(name);
  if (!factory || !factory->create_runloop) {
    return NULL;
  }

  impl = factory->create_runloop();
  axis_ASSERT(impl, "Failed to create %s runloop implementation.", name);
  if (!impl) {
    return NULL;
  }

  runloop_init(impl, 0);

  return &impl->base;
}

void axis_runloop_destroy(axis_runloop_t *loop) {
  axis_ASSERT(loop &&
                 // axis_NOLINTNEXTLINE(thread-check)
                 // thread-check: destroying might be occurred in any threads.
                 axis_runloop_check_integrity(loop, false),
             "Invalid argument.");

  axis_runloop_common_t *impl = (axis_runloop_common_t *)loop;
  if (!loop) {
    return;
  }

  axis_ASSERT(!impl->task_available_signal, "Should not happen.");

  axis_mutex_destroy(impl->lock);
  impl->lock = NULL;

  if (impl->destroy) {
    impl->destroy(&impl->base);
  }
}

axis_runloop_t *axis_runloop_current(void) { return get_self(); }

axis_runloop_t *axis_runloop_attach(const char *type, void *raw) {
  const char *name = type ? type : get_default_impl();
  axis_runloop_common_t *impl = NULL;
  const runloop_factory_t *factory = NULL;

  axis_thread_once(&runloop_once, setup_runloop_callback);

  factory = get_runloop_factory(name);
  if (!factory || !factory->attach) {
    return NULL;
  }

  impl = factory->attach(raw);

  if (!impl) {
    return NULL;
  }

  runloop_init(impl, 1);

  return &impl->base;
}

bool axis_runloop_is_attached(axis_runloop_t *loop) {
  axis_runloop_common_t *impl = (axis_runloop_common_t *)loop;
  axis_ASSERT(impl && axis_runloop_check_integrity(loop, true),
             "Invalid argument.");

  return impl->attach_other;
}

void *axis_runloop_get_raw(axis_runloop_t *loop) {
  axis_runloop_common_t *impl = (axis_runloop_common_t *)loop;
  axis_ASSERT(impl &&
                 // axis_NOLINTNEXTLINE(thread-check)
                 // thread-check: This function is intended to be called in any
                 // threads.
                 axis_runloop_check_integrity(loop, false),
             "Invalid argument.");

  void *raw = NULL;

  axis_mutex_lock(impl->lock);

  if (!loop || !impl->get_raw) {
    goto done;
  }
  raw = impl->get_raw(loop);

done:
  axis_mutex_unlock(impl->lock);
  return raw;
}

void axis_runloop_run(axis_runloop_t *loop) {
  axis_ASSERT(loop && axis_runloop_check_integrity(loop, true),
             "Invalid argument.");

  axis_runloop_common_t *impl = (axis_runloop_common_t *)loop;

  // If the underlying is created separately, it will start running by itself,
  // so we do _not_ need to enable it to run here.
  if (!loop || !impl->run || axis_atomic_load(&impl->attach_other)) {
    return;
  }

  set_self(loop);
  axis_atomic_store((axis_atomic_t *)&impl->state, axis_RUNLOOP_STATE_RUNNING);

  impl->run(loop);

  axis_atomic_store((axis_atomic_t *)&impl->state, axis_RUNLOOP_STATE_IDLE);
  set_self(NULL);
}

/**
 * @brief This function is used to 'release' the resources occupied by the
 * runloop internally, therefore, we should _not_ close the runloop before it
 * stops.
 */
void axis_runloop_close(axis_runloop_t *loop) {
  axis_ASSERT(loop && axis_runloop_check_integrity(loop, true),
             "Invalid argument.");

  axis_runloop_common_t *impl = (axis_runloop_common_t *)loop;

  if (!loop || !impl->close) {
    return;
  }
  impl->close(loop);
}

void axis_runloop_stop(axis_runloop_t *loop) {
  axis_ASSERT(loop && axis_runloop_check_integrity(loop, true),
             "Invalid argument.");

  axis_runloop_common_t *impl = (axis_runloop_common_t *)loop;
  if (!loop || !impl->stop) {
    return;
  }

  axis_mutex_lock(impl->lock);
  impl->destroying = 1;
  axis_mutex_unlock(impl->lock);

  axis_runloop_async_close(impl->task_available_signal,
                          task_available_signal_closed);
}

void axis_runloop_set_on_stopped(axis_runloop_t *loop,
                                axis_runloop_on_stopped_func_t on_stopped,
                                void *on_stopped_data) {
  axis_ASSERT(loop && axis_runloop_check_integrity(loop, true),
             "Invalid argument.");

  axis_runloop_common_t *impl = (axis_runloop_common_t *)loop;
  impl->on_stopped = on_stopped;
  impl->on_stopped_data = on_stopped_data;
}

int axis_runloop_alive(axis_runloop_t *loop) {
  axis_ASSERT(loop && axis_runloop_check_integrity(loop, true),
             "Invalid argument.");

  axis_runloop_common_t *impl = (axis_runloop_common_t *)loop;

  if (!impl || !impl->alive) {
    return 0;
  }

  return impl->alive(loop);
}

axis_runloop_async_t *axis_runloop_async_create(const char *type) {
  const char *name = type ? type : get_default_impl();
  axis_runloop_async_common_t *impl = NULL;
  const runloop_factory_t *factory = NULL;

  axis_thread_once(&runloop_once, setup_runloop_callback);

  factory = get_runloop_factory(name);
  if (!factory || !factory->create_async) {
    return NULL;
  }

  impl = factory->create_async();
  axis_ASSERT(impl, "Failed to create %s async.", name);
  if (!impl) {
    return NULL;
  }

  impl->base.loop = NULL;

  axis_signature_set(&impl->base.signature, axis_RUNLOOP_ASYNC_SIGNATURE);
  axis_sanitizer_thread_check_init_with_current_thread(&impl->base.thread_check);

  return &impl->base;
}

void axis_runloop_async_close(axis_runloop_async_t *async,
                             void (*close_cb)(axis_runloop_async_t *)) {
  axis_ASSERT(async && axis_runloop_async_check_integrity(async, true),
             "Invalid argument.");

  axis_runloop_async_common_t *impl = (axis_runloop_async_common_t *)async;

  if (!async || !impl->close) {
    return;
  }

  impl->close(async, close_cb);
}

void axis_runloop_async_destroy(axis_runloop_async_t *async) {
  axis_ASSERT(async && axis_runloop_async_check_integrity(async, true),
             "Invalid argument.");

  axis_runloop_async_common_t *impl = (axis_runloop_async_common_t *)async;

  if (!async || !impl->destroy) {
    return;
  }

  impl->destroy(async);
}

int axis_runloop_async_notify(axis_runloop_async_t *async) {
  axis_ASSERT(async &&
                 // axis_NOLINTNEXTLINE(thread-check)
                 // thread-check: This function is intended to be called in any
                 // threads.
                 axis_runloop_async_check_integrity(async, false),
             "Invalid argument.");

  axis_runloop_async_common_t *impl = (axis_runloop_async_common_t *)async;

  if (!async || !impl->notify) {
    return -1;
  }

  return impl->notify(async);
}

int axis_runloop_async_init(axis_runloop_async_t *async, axis_runloop_t *loop,
                           void (*callback)(axis_runloop_async_t *)) {
  axis_ASSERT(async && axis_runloop_async_check_integrity(async, true),
             "Invalid argument.");
  axis_ASSERT(loop && axis_runloop_check_integrity(loop, true),
             "Invalid argument.");

  axis_runloop_async_common_t *impl = (axis_runloop_async_common_t *)async;
  int ret = -1;

  if (!async || !impl->init) {
    return -1;
  }

  if (impl->base.loop) {
    return -1;
  }

  if (strcmp(async->impl, loop->impl) != 0) {
    return -1;
  }

  ret = impl->init(async, loop, callback);
  if (ret == 0) {
    async->loop = loop;
  }

  return ret;
}

static int axis_runloop_post_task_at(axis_runloop_t *loop,
                                    void (*task_cb)(void *, void *), void *from,
                                    void *arg, int front) {
  axis_ASSERT(loop &&
                 // axis_NOLINTNEXTLINE(thread-check)
                 // thread-check: This function is intended to be called in any
                 // threads.
                 axis_runloop_check_integrity(loop, false),
             "Invalid argument.");

  axis_runloop_common_t *impl = (axis_runloop_common_t *)loop;
  axis_runloop_task_t *task = NULL;
  int needs_notify = 0;

  if (!loop || !task_cb) {
    goto error;
  }

  task = (axis_runloop_task_t *)axis_MALLOC(sizeof(axis_runloop_task_t));
  axis_ASSERT(task, "Failed to allocate memory.");
  if (!task) {
    goto error;
  }

  memset(task, 0, sizeof(axis_runloop_task_t));
  task->func = task_cb;
  task->from = from;
  task->arg = arg;

  axis_UNUSED bool rc = axis_mutex_lock(impl->lock);
  axis_ASSERT(!rc, "Failed to lock.");

  if (impl->destroying) {
    // The runloop has started to close, so we do _not_ add any more new tasks
    // into it.
    goto leave_and_error;
  }

  needs_notify = axis_list_is_empty(&impl->tasks) ? 1 : 0;
  if (front) {
    axis_list_push_front(&impl->tasks, &task->node);
  } else {
    axis_list_push_back(&impl->tasks, &task->node);
  }

  rc = axis_mutex_unlock(impl->lock);
  axis_ASSERT(!rc, "Failed to unlock.");

  if (needs_notify) {
    axis_runloop_async_notify(impl->task_available_signal);
  }

  return 0;

leave_and_error:
  rc = axis_mutex_unlock(impl->lock);
  axis_ASSERT(!rc, "Failed to unlock.");

error:
  if (task) {
    axis_FREE(task);
  }

  return -1;
}

int axis_runloop_post_task_front(axis_runloop_t *loop,
                                axis_runloop_task_func_t task_cb, void *from,
                                void *arg) {
  axis_ASSERT(loop &&
                 // axis_NOLINTNEXTLINE(thread-check)
                 // thread-check: This function is intended to be called in any
                 // threads.
                 axis_runloop_check_integrity(loop, false),
             "Invalid argument.");
  return axis_runloop_post_task_at(loop, task_cb, from, arg, 1);
}

int axis_runloop_post_task_tail(axis_runloop_t *loop,
                               axis_runloop_task_func_t task_cb, void *from,
                               void *arg) {
  axis_ASSERT(loop &&
                 // axis_NOLINTNEXTLINE(thread-check)
                 // thread-check: This function is intended to be called in any
                 // threads.
                 axis_runloop_check_integrity(loop, false),
             "Invalid argument.");
  return axis_runloop_post_task_at(loop, task_cb, from, arg, 0);
}

size_t axis_runloop_task_queue_size(axis_runloop_t *loop) {
  axis_ASSERT(loop &&
                 // axis_NOLINTNEXTLINE(thread-check)
                 // thread-check: This function is intended to be called in any
                 // threads.
                 axis_runloop_check_integrity(loop, false),
             "Invalid argument.");

  axis_runloop_common_t *impl = (axis_runloop_common_t *)loop;

  if (!loop) {
    return -1;
  }

  axis_mutex_lock(impl->lock);
  size_t size = axis_list_size(&impl->tasks);
  axis_mutex_unlock(impl->lock);

  return size;
}

void axis_runloop_flush_task(axis_runloop_t *loop) {
  axis_ASSERT(loop &&
                 // axis_NOLINTNEXTLINE(thread-check)
                 // thread-check: This function is intended to be called in any
                 // threads.
                 axis_runloop_check_integrity(loop, false),
             "Invalid argument.");

  flush_remaining_tasks((axis_runloop_common_t *)loop);
}

axis_runloop_timer_t *axis_runloop_timer_create(const char *type,
                                              uint64_t timeout,
                                              uint64_t periodic) {
  const char *name = type ? type : get_default_impl();
  axis_runloop_timer_common_t *impl = NULL;
  const runloop_factory_t *factory = NULL;

  axis_thread_once(&runloop_once, setup_runloop_callback);

  factory = get_runloop_factory(name);
  if (!factory || !factory->create_timer) {
    return NULL;
  }

  impl = factory->create_timer();
  axis_ASSERT(impl, "Failed to create %s timer.", name);
  if (!impl) {
    return NULL;
  }

  axis_signature_set(&impl->base.signature, axis_RUNLOOP_TIMER_SIGNATURE);
  axis_sanitizer_thread_check_init_with_current_thread(&impl->base.thread_check);

  impl->base.loop = NULL;
  impl->base.timeout = timeout;
  impl->base.periodic = periodic;

  return &impl->base;
}

int axis_runloop_timer_set_timeout(axis_runloop_timer_t *timer, uint64_t timeout,
                                  uint64_t periodic) {
  axis_ASSERT(timer && axis_runloop_timer_check_integrity(timer, true),
             "Invalid argument.");

  axis_runloop_timer_common_t *impl = (axis_runloop_timer_common_t *)timer;

  if (!timer) {
    return -1;
  }

  impl->base.timeout = timeout;
  impl->base.periodic = periodic;
  return 0;
}

void axis_runloop_timer_stop(axis_runloop_timer_t *timer,
                            void (*stop_cb)(axis_runloop_timer_t *, void *),
                            void *arg) {
  axis_ASSERT(timer && axis_runloop_timer_check_integrity(timer, true),
             "Invalid argument.");

  axis_runloop_timer_common_t *impl = (axis_runloop_timer_common_t *)timer;

  if (!timer || !impl->stop) {
    return;
  }

  impl->stop_data = arg;
  impl->stop(timer, stop_cb);
}

void axis_runloop_timer_close(axis_runloop_timer_t *timer,
                             void (*close_cb)(axis_runloop_timer_t *, void *),
                             void *arg) {
  axis_ASSERT(timer && axis_runloop_timer_check_integrity(timer, true),
             "Invalid argument.");

  axis_runloop_timer_common_t *impl = (axis_runloop_timer_common_t *)timer;

  if (!timer || !impl->close) {
    return;
  }

  impl->close_data = arg;
  impl->close(timer, close_cb);
}

void axis_runloop_timer_destroy(axis_runloop_timer_t *timer) {
  axis_ASSERT(timer && axis_runloop_timer_check_integrity(timer, true),
             "Invalid argument.");

  axis_runloop_timer_common_t *impl = (axis_runloop_timer_common_t *)timer;

  if (!timer || !impl->destroy) {
    return;
  }

  impl->destroy(timer);
}

int axis_runloop_timer_start(axis_runloop_timer_t *timer, axis_runloop_t *loop,
                            void (*callback)(axis_runloop_timer_t *, void *),
                            void *arg) {
  axis_ASSERT(timer && axis_runloop_timer_check_integrity(timer, true),
             "Invalid argument.");
  axis_ASSERT(loop && axis_runloop_check_integrity(loop, true),
             "Invalid argument.");

  axis_runloop_timer_common_t *impl = (axis_runloop_timer_common_t *)timer;
  int ret = -1;

  if (!timer || !impl->start) {
    return -1;
  }

  if (strcmp(timer->impl, loop->impl) != 0) {
    return -1;
  }

  impl->start_data = arg;
  ret = impl->start(timer, loop, callback);
  if (ret == 0) {
    timer->loop = loop;
  }

  return ret;
}
