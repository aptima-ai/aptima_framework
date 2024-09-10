//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include "ten_utils/lib/thread_once.h"

#include <pthread.h>

#include "ten_utils/log/log.h"
#include "ten_utils/macro/check.h"

int ten_thread_once(ten_thread_once_t *once_control,
                    void (*init_routine)(void)) {
  TEN_ASSERT(once_control && init_routine, "Invalid argument.");

  int rc = pthread_once(once_control, init_routine);
  if (rc != 0) {
    TEN_LOGE("Failed to pthread_once(): %d", rc);
  }

  return rc;
}
