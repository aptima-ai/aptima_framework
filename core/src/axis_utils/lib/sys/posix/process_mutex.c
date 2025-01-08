//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_utils/lib/process_mutex.h"

#include <errno.h>
#include <fcntl.h>
#include <semaphore.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "axis_utils/lib/alloc.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/log/log.h"
#include "axis_utils/macro/check.h"

#define axis_PROCESS_MUTEX_CREATE_MODE 0644

typedef struct axis_process_mutex_t {
  sem_t *sem;
  axis_string_t *name;
} axis_process_mutex_t;

axis_process_mutex_t *axis_process_mutex_create(const char *name) {
  axis_ASSERT(name, "Invalid argument.");

  sem_t *sem = sem_open(name, O_CREAT, axis_PROCESS_MUTEX_CREATE_MODE, 1);
  if (!sem) {
    axis_LOGE("Failed to sem_open: %d", errno);
    return NULL;
  }

  axis_process_mutex_t *mutex = axis_MALLOC(sizeof(axis_process_mutex_t));
  axis_ASSERT(mutex, "Failed to allocate memory.");

  mutex->sem = sem;
  mutex->name = axis_string_create_formatted("%s", name);

  return mutex;
}

int axis_process_mutex_lock(axis_process_mutex_t *mutex) {
  axis_ASSERT(mutex, "Invalid argument.");

  return sem_wait(mutex->sem);
}

int axis_process_mutex_unlock(axis_process_mutex_t *mutex) {
  axis_ASSERT(mutex, "Invalid argument.");

  return sem_post(mutex->sem);
}

void axis_process_mutex_destroy(axis_process_mutex_t *mutex) {
  axis_ASSERT(mutex, "Invalid argument.");

  int ret = sem_close(mutex->sem);
  if (ret) {
    axis_LOGE("Failed to sem_close: %d", errno);
  }
  axis_ASSERT(!ret, "Should not happen.");

  // For the purpose of process mutex, should never call sem_unlink(), because
  // calls to sem_open() to re-create or re-connect to the specified semaphore
  // refer to a new semaphore after sem_unlink() is called. Thus the process
  // mutex mechanism would lose efficacy.
  //
  // Reference from https://www.mkssoftware.com/docs/man3/sem_unlink.3.asp
  //
  // And, never call sem_unlink() will cause file leaks under path /dev/shm
  // before next system restart.
  //
  // TODO(ZhangXianyao): for some use cases of process mutex, such as
  // modifing a file at the same time, recommend using file-lock instead.
  //
  // sem_unlink(axis_string_get_raw_str(mutex->name));

  axis_string_destroy(mutex->name);
  axis_FREE(mutex);
}
