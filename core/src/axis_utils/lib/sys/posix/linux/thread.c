//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//

#include "axis_utils/lib/thread.h"

#include "axis_utils/lib/atomic.h"

#define _GNU_SOURCE
#define __USE_GNU
#include <pthread.h>
#include <sched.h>
#include <sys/syscall.h>
#include <unistd.h>

axis_tid_t axis_thread_get_id(axis_thread_t *thread) {
  if (!thread) {
    return syscall(__NR_gettid);
  }

  return axis_atomic_load(&thread->id);
}

int axis_thread_suspend(axis_thread_t *thread) {
  // Not possible in linux
  (void)thread;
  return -1;
}

int axis_thread_resume(axis_thread_t *thread) {
  // Not possible in linux
  (void)thread;
  return -1;
}
