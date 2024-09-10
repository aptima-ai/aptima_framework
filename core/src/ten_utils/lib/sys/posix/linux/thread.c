//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//

#include "ten_utils/lib/thread.h"

#include "ten_utils/lib/atomic.h"

#define _GNU_SOURCE
#define __USE_GNU
#include <pthread.h>
#include <sched.h>
#include <sys/syscall.h>
#include <unistd.h>

ten_tid_t ten_thread_get_id(ten_thread_t *thread) {
  if (!thread) {
    return syscall(__NR_gettid);
  }

  return ten_atomic_load(&thread->id);
}

int ten_thread_suspend(ten_thread_t *thread) {
  // Not possible in linux
  (void)thread;
  return -1;
}

int ten_thread_resume(ten_thread_t *thread) {
  // Not possible in linux
  (void)thread;
  return -1;
}
