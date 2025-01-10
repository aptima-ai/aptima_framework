//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_utils/lib/thread_once.h"

#include <pthread.h>

#include "axis_utils/log/log.h"
#include "axis_utils/macro/check.h"

int axis_thread_once(axis_thread_once_t *once_control,
                    void (*init_routine)(void)) {
  axis_ASSERT(once_control && init_routine, "Invalid argument.");

  int rc = pthread_once(once_control, init_routine);
  if (rc != 0) {
    axis_LOGE("Failed to pthread_once(): %d", rc);
  }

  return rc;
}
