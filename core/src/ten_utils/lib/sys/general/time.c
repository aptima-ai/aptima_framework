//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include "ten_utils/lib/time.h"

#include <stdlib.h>

#include "ten_utils/lib/random.h"

void ten_random_sleep(const int64_t max_msec) {
  int64_t wait_time = 0;
  ten_random(&wait_time, sizeof(wait_time));

  // ten_random might give us a negative number, so we need to take an action to
  // ensure it is positive.
  wait_time = llabs(wait_time);

  wait_time %= max_msec;
  ten_sleep(wait_time);
}
