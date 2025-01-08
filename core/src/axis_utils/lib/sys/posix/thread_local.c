//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_utils/lib/thread_local.h"

#include <pthread.h>

#include "axis_utils/log/log.h"

axis_thread_key_t axis_thread_key_create(void) {
  axis_thread_key_t key = kInvalidTlsKey;

  if (pthread_key_create(&key, NULL) != 0) {
    axis_LOGE("Failed to create a key in thread local storage.");
    return kInvalidTlsKey;
  }

  return key;
}

void axis_thread_key_destroy(axis_thread_key_t key) {
  if (key == kInvalidTlsKey) {
    axis_LOGE("Invalid argument for thread local storage key.");
    return;
  }

  pthread_key_delete(key);
}

int axis_thread_set_key(axis_thread_key_t key, void *value) {
  if (key == kInvalidTlsKey) {
    axis_LOGE("Invalid argument for thread local storage key.");
    return -1;
  }

  int rc = pthread_setspecific(key, value);
  if (rc == 0) {
    return 0;
  } else {
    axis_LOGE("Failed to pthread_setspecific: %d", rc);
    return -1;
  }
}

void *axis_thread_get_key(axis_thread_key_t key) {
  if (key == kInvalidTlsKey) {
    axis_LOGE("Invalid argument for thread local storage key.");
    return NULL;
  }

  return pthread_getspecific(key);
}
