//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_utils/lib/time.h"

#include <stdlib.h>

#include "axis_utils/lib/random.h"

void axis_random_sleep(const int64_t max_msec) {
  int64_t wait_time = 0;
  axis_random(&wait_time, sizeof(wait_time));

  // axis_random might give us a negative number, so we need to take an action to
  // ensure it is positive.
  wait_time = llabs(wait_time);

  wait_time %= max_msec;
  axis_sleep(wait_time);
}
