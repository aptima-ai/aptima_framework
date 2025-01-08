//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_utils/lib/thread.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "axis_utils/log/log.h"

#if defined(__linux__)
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#include "axis_utils/lib/alloc.h"
#include "axis_utils/lib/thread_local.h"
#include "axis_utils/lib/thread_once.h"
#include "axis_utils/macro/check.h"

static axis_thread_once_t __tcb_once = axis_THREAD_ONCE_INIT;
static axis_thread_key_t __tcb = kInvalidTlsKey;

static void __setup_tcb_callback(void) { __tcb = axis_thread_key_create(); }

axis_thread_t *__get_self() {
  axis_thread_key_t tcb = __tcb;

  if (tcb == kInvalidTlsKey) {
    return NULL;
  }

  return axis_thread_get_key(tcb);
}

static int __set_self(axis_thread_t *self) {
  axis_thread_key_t tcb = __tcb;

  if (tcb == kInvalidTlsKey) {
    return -1;
  }

  return axis_thread_set_key(tcb, self);
}

static void __dealloc_axis_thread(axis_thread_t **t) {
  axis_ASSERT(t, "Invalid argument.");
  if (!t || !*t) {
    return;
  }

  if ((*t)->ready) {
    axis_event_destroy((*t)->ready);
  }

  if ((*t)->exit) {
    axis_event_destroy((*t)->exit);
  }

  if ((*t)->name) {
    axis_FREE((*t)->name);
  }

#if defined(_WIN32)
  if ((*t)->aux) {
    CloseHandle((HANDLE)(*t)->aux);
  }
#endif

  axis_FREE((*t));

  *t = NULL;
}

#if defined(_WIN32)
static DWORD WINAPI win32_routine(_In_ LPVOID args) {
#else
static void *pthread_routine(void *args) {
#endif
  axis_thread_t *t = (axis_thread_t *)args;
  void *routine_result = NULL;

  // axis_LOGD("pthread_routine (%s) start.", t->name);

  if (!args) {
    // So not possible but just in case
    axis_ASSERT(0, "Should not happen.");
    // Nothing we can do and memory leaks
    return 0;
  }

  // Setup TCB so `axis_thread_self()` can work
  __set_self(t);

  // Get self id
  axis_atomic_store(&t->id, axis_thread_get_id(NULL));

#if defined(_WIN32)
  // Setup thread handle for future usage
  t->aux = (void *)OpenThread(THREAD_ALL_ACCESS, FALSE, (DWORD)t->id);
#else
  t->aux = (void *)pthread_self();
#endif

  // Setup name for this thread
  if (t->name) {
    int rc = axis_thread_set_name(NULL, t->name);
    if (rc) {
      axis_LOGW("Failed to set thread name: %s", t->name);
    }
  }

  // Notify ready so `axis_thread_create()` can continue
  if (t->ready) {
    axis_event_set(t->ready);
  }

  // Run actual routine and get result
  if (t->routine) {
    routine_result = t->routine(t->args);
  }

  __set_self(NULL);

  // Now safe to dealloc (if needed)
  if (axis_atomic_load(&t->detached) == 1) {
    __dealloc_axis_thread(&t);
  } else {
    // Notify exit so `axis_thread_join()` can work
    if (t->exit) {
      axis_event_set(t->exit);
    }
  }

  // axis_LOGD("pthread_routine end.");

  return 0;
}

axis_thread_t *axis_thread_create(const char *name,
                                void *(*axis_thread_routine)(void *),
                                void *args) {
  axis_thread_t *thread = NULL;

  if (!axis_thread_routine) {
    goto error;
  }

  thread = (axis_thread_t *)axis_MALLOC(sizeof(*thread));
  axis_ASSERT(thread, "Failed to allocate memory.");

  if (!thread) {
    goto error;
  }

  memset(thread, 0, sizeof(*thread));

  if (axis_thread_once(&__tcb_once, __setup_tcb_callback) != 0) {
    axis_LOGE("Failed to axis_thread_once.");
    return NULL;
  }

  thread->args = args;
  thread->routine = axis_thread_routine;
  thread->id = 0;
  thread->ready = axis_event_create(0, 0);
  thread->exit = axis_event_create(0, 0);
  if (name) {
    thread->name = strdup(name);
  } else {
    thread->name = NULL;
  }

#if defined(_WIN32)
  if (CreateThread(NULL, 0, win32_routine, thread, 0, NULL) == NULL) {
    goto error;
  }
#else
  pthread_t self = 0;

  pthread_attr_t *attr_ptr = NULL;

#if defined(__i386__) || defined(__arm__)
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  size_t stacksize = 256UL * 1024UL;
  pthread_attr_setstacksize(&attr, stacksize);

  attr_ptr = &attr;
#endif

  int rc = pthread_create(&self, attr_ptr, pthread_routine, thread);
  if (rc != 0) {
    axis_LOGE("Failed to pthread_create: %d", rc);
    axis_ASSERT(0, "Should not happen.");
    goto error;
  }

#if defined(_DEBUG) && defined(__linux__)
  axis_LOGV("New thread is created, id(%lu), name(%s).", syscall(__NR_gettid),
           name);
#endif

#endif

  axis_event_wait(thread->ready, -1);

  return thread;

error:
  __dealloc_axis_thread(&thread);
  return NULL;
}

int axis_thread_join(axis_thread_t *thread, int wait_ms) {
  if (!thread) {
    axis_LOGE("Invalid argument: thread is NULL.");
    return -1;
  }

  if (!thread->exit) {
    axis_LOGE("Failed to join thead because it's exit is 0.");
    axis_ASSERT(0, "Should not happen.");
    return -1;
  }

  if (axis_event_wait(thread->exit, wait_ms) == 0) {
    if (axis_atomic_load(&thread->detached) == 0) {
      __dealloc_axis_thread(&thread);
    }
  }

  return 0;
}

int axis_thread_detach(axis_thread_t *thread) {
  axis_ASSERT(thread, "Invalid argument.");
  if (!thread) {
    return -1;
  }

  axis_atomic_store(&thread->detached, 1);
  return 0;
}

axis_thread_t *axis_thread_create_fake(const char *name) {
  axis_thread_t *t = (axis_thread_t *)axis_MALLOC(sizeof(axis_thread_t));
  axis_ASSERT(t, "Failed to allocate memory.");

  memset(t, 0, sizeof(axis_thread_t));

  if (name) {
    t->name = strdup(name);
  } else {
    t->name = NULL;
  }

  // Get self id
  axis_atomic_store(&t->id, axis_thread_get_id(NULL));

#if defined(_WIN32)
  // Setup thread handle for future usage
  t->aux = (void *)OpenThread(THREAD_ALL_ACCESS, FALSE, (DWORD)t->id);
#else
  t->aux = (void *)pthread_self();

#if defined(_DEBUG) && defined(__linux__)
  axis_LOGV("New thread is created, id(%lu), name(%s).", syscall(__NR_gettid),
           name);
#endif

#endif

  return t;
}

int axis_thread_join_fake(axis_thread_t *thread) {
  axis_ASSERT(thread, "Invalid argument.");

  __dealloc_axis_thread(&thread);
  return 0;
}

const char *axis_thread_get_name(axis_thread_t *thread) {
  if (!thread) {
    thread = axis_thread_self();
  }

  // 'axis_thread_self()' may returns NULL.
  if (!thread) {
    return NULL;
  }

  return thread->name;
}
