//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_utils/lib/process_mutex.h"

#include <Windows.h>
#include <assert.h>
#include <stdlib.h>

#include "axis_utils/lib/string.h"

#define axis_PROCESS_MUTEX_CREATE_MODE 0644

typedef struct axis_process_mutex_t {
  void *handle;
  axis_string_t *name;
} axis_process_mutex_t;

axis_process_mutex_t *axis_process_mutex_create(const char *name) {
  assert(name);

  axis_process_mutex_t *mutex = malloc(sizeof(axis_process_mutex_t));
  assert(mutex);

  mutex->handle = NULL;
  mutex->handle = CreateMutex(NULL, false, name);
  assert(mutex->handle);
  mutex->name = axis_string_create_formatted("%s", name);

  return mutex;
}

int axis_process_mutex_lock(axis_process_mutex_t *mutex) {
  assert(mutex);

  DWORD ret = WaitForSingleObject(mutex->handle, INFINITE);
  if (ret != WAIT_OBJECT_0) {
    return -1;
  }

  return 0;
}

int axis_process_mutex_unlock(axis_process_mutex_t *mutex) {
  assert(mutex);

  if (ReleaseMutex(mutex->handle)) {
    return 0;
  }

  return -1;
}

void axis_process_mutex_destroy(axis_process_mutex_t *mutex) {
  assert(mutex);

  CloseHandle(mutex->handle);
  axis_string_destroy(mutex->name);
  free(mutex);
}
