//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "ten_utils/lib/time.h"

#include <errno.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

int64_t ten_current_time(void) {
  struct timeval tv;
  gettimeofday(&tv, NULL);

  // convert tv_sec & tv_usec to millisecond
  return ((int64_t)tv.tv_sec) * 1000 + ((int64_t)tv.tv_usec) / 1000;
}

int64_t ten_current_time_us(void) {
  struct timeval tv;
  gettimeofday(&tv, NULL);

  // convert tv_sec & tv_usec to microseconds
  return ((int64_t)tv.tv_sec) * 1000000 + (int64_t)tv.tv_usec;
}

// Sleep for the requested number of milliseconds.
void ten_sleep(const int64_t msec) {
  struct timespec ts;
  ts.tv_sec = msec / 1000;
  ts.tv_nsec = (msec % 1000) * 1000000;

  int res = 0;
  do {
    res = nanosleep(&ts, &ts);
  } while (res && errno == EINTR);
}

void ten_usleep(int64_t usec) { usleep(usec); }
