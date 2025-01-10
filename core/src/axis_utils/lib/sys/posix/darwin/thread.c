//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//

#include "axis_utils/lib/thread.h"

#include <mach/mach.h>
#include <mach/thread_act.h>
#include <mach/thread_policy.h>
#include <pthread.h>
#include <sys/_pthread/_pthread_t.h>

#include "axis_utils/lib/atomic.h"

axis_tid_t axis_thread_get_id(axis_thread_t *thread) {
  uint64_t id = 0;

  if (!thread) {
    if (pthread_threadid_np(NULL, &id) != 0) {
      return -1;
    }
    return (axis_tid_t)id;
  }

  return axis_atomic_load(&thread->id);
}

int axis_thread_suspend(axis_thread_t *thread) {
  mach_port_t mach_thread = 0;

  if (thread && !thread->aux) {
    return -1;
  }

  if (!thread) {
    mach_thread = mach_thread_self();
  } else {
    mach_thread = pthread_mach_thread_np(thread->aux);
  }

  if (thread_suspend(mach_thread) != KERN_SUCCESS) {
    return -1;
  }

  return 0;
}

int axis_thread_resume(axis_thread_t *thread) {
  mach_port_t mach_thread = 0;

  if (thread && !thread->aux) {
    return -1;
  }

  if (!thread) {
    mach_thread = mach_thread_self();
  } else {
    mach_thread = pthread_mach_thread_np(thread->aux);
  }

  if (thread_resume(mach_thread) != KERN_SUCCESS) {
    return -1;
  }

  return 0;
}

int axis_thread_set_name(axis_thread_t *thread, const char *name) {
  if (!name || !*name) {
    return -1;
  }

  if (thread && thread != axis_thread_self()) {
    // Darwin posix thread can only set name for self
    return -1;
  }

  return pthread_setname_np(name);
}

void axis_thread_set_affinity(axis_thread_t *thread, uint64_t mask) {
  mach_port_t mach_thread = 0;
  integer_t policy = (integer_t)(uint32_t)mask;

  if (thread && !thread->aux) {
    return;
  }

  if (!thread) {
    mach_thread = mach_thread_self();
  } else {
    mach_thread = pthread_mach_thread_np(thread->aux);
  }

  thread_policy_set(mach_thread, THREAD_AFFINITY_POLICY, &policy,
                    THREAD_AFFINITY_POLICY_COUNT);
}
