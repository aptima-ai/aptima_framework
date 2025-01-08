//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_utils/lib/spinlock.h"

#include <memory.h>

#include "axis_utils/lib/atomic.h"

void axis_spinlock_init(axis_spinlock_t *spin) {
  static const axis_spinlock_t initializer = axis_SPINLOCK_INIT;
  *spin = initializer;
}

axis_spinlock_t *axis_spinlock_from_addr(axis_atomic_t *addr) {
  axis_spinlock_t *spin = (axis_spinlock_t *)addr;
  axis_spinlock_init(spin);
  return spin;
}

void axis_spinlock_lock(axis_spinlock_t *spin) {
  uint64_t loops = 0;
  while (axis_atomic_test_set(&spin->lock, 1) == 1) {
    if (loops++ > 200) {
      axis_thread_pause_cpu();
    }
  }
}

int axis_spinlock_trylock(axis_spinlock_t *spin) {
  return (axis_atomic_test_set(&spin->lock, 1) != 1) ? 1 : 0;
}

void axis_spinlock_unlock(axis_spinlock_t *spin) {
  axis_memory_barrier();
  spin->lock = 0;
  axis_memory_barrier();
}

void axis_recursive_spinlock_init(axis_recursive_spinlock_t *spin) {
  static const axis_recursive_spinlock_t initializer =
      axis_RECURSIVE_SPINLOCK_INIT;
  *spin = initializer;
}

axis_recursive_spinlock_t *axis_recursive_spinlock_from_addr(uint8_t *addr) {
  axis_recursive_spinlock_t *spin = (axis_recursive_spinlock_t *)addr;
  axis_recursive_spinlock_init(spin);
  return spin;
}

void axis_recursive_spinlock_lock(axis_recursive_spinlock_t *spin) {
  axis_tid_t tid = axis_thread_get_id(NULL);
  axis_pid_t pid = axis_task_get_id();

  if (spin->tid != tid || spin->pid != pid) {
    axis_spinlock_lock(&spin->lock);
    spin->pid = pid;
    spin->tid = tid;
  }

  spin->count++;
}

int axis_recursive_spinlock_trylock(axis_recursive_spinlock_t *spin) {
  axis_tid_t tid = axis_thread_get_id(NULL);
  axis_pid_t pid = axis_task_get_id();

  if (spin->tid != tid || spin->pid != pid) {
    if (axis_spinlock_trylock(&spin->lock) == 0) {
      return 0;
    }

    spin->tid = tid;
    spin->pid = pid;
  }

  spin->count++;
  return 1;
}

void axis_recursive_spinlock_unlock(axis_recursive_spinlock_t *spin) {
  if (--(spin->count) == 0) {
    spin->tid = -1;
    spin->pid = -1;
    axis_spinlock_unlock(&spin->lock);
  }
}
