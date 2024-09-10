//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include "ten_utils/lib/thread_local.h"

#include <pthread.h>

#include "ten_utils/log/log.h"

ten_thread_key_t ten_thread_key_create(void) {
  ten_thread_key_t key = kInvalidTlsKey;

  if (pthread_key_create(&key, NULL) != 0) {
    TEN_LOGE("Failed to create a key in thread local storage.");
    return kInvalidTlsKey;
  }

  return key;
}

void ten_thread_key_destroy(ten_thread_key_t key) {
  if (key == kInvalidTlsKey) {
    TEN_LOGE("Invalid argument for thread local storage key.");
    return;
  }

  pthread_key_delete(key);
}

int ten_thread_set_key(ten_thread_key_t key, void *value) {
  if (key == kInvalidTlsKey) {
    TEN_LOGE("Invalid argument for thread local storage key.");
    return -1;
  }

  int rc = pthread_setspecific(key, value);
  if (rc == 0) {
    return 0;
  } else {
    TEN_LOGE("Failed to pthread_setspecific: %d", rc);
    return -1;
  }
}

void *ten_thread_get_key(ten_thread_key_t key) {
  if (key == kInvalidTlsKey) {
    TEN_LOGE("Invalid argument for thread local storage key.");
    return NULL;
  }

  return pthread_getspecific(key);
}
