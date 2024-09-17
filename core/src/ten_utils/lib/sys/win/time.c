//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "ten_utils/lib/time.h"

#include <Windows.h>

int64_t ten_current_time(void) {
  FILETIME ft;
  LARGE_INTEGER li;

  /* Get the amount of 100 nano seconds intervals elapsed since January 1, 1601
   * (UTC) and copy it to a LARGE_INTEGER structure. */
  GetSystemTimeAsFileTime(&ft);
  li.LowPart = ft.dwLowDateTime;
  li.HighPart = ft.dwHighDateTime;

  uint64_t ret = li.QuadPart;
  ret -= 116444736000000000LL; /* Convert from file time to UNIX epoch time. */
  ret /= 10000; /* From 100 nano seconds (10^-7) to 1 millisecond (10^-3)
                   intervals */

  return (int64_t)ret;
}

int64_t ten_current_time_us(void) {
  FILETIME ft;
  LARGE_INTEGER li;

  /* Get the amount of 100 nano seconds intervals elapsed since January 1, 1601
   * (UTC) and copy it to a LARGE_INTEGER structure. */
  GetSystemTimeAsFileTime(&ft);
  li.LowPart = ft.dwLowDateTime;
  li.HighPart = ft.dwHighDateTime;

  uint64_t ret = li.QuadPart;
  ret -= 116444736000000000LL; /* Convert from file time to UNIX epoch time. */
  ret /= 10; /* From 100 nano seconds (10^-7) to 1 microsecond (10^-6)
                   intervals */

  return (int64_t)ret;
}

// Sleep for the requested number of milliseconds.
void ten_sleep(int64_t msec) { Sleep(msec); }

void ten_usleep(int64_t usec) {
  HANDLE timer;
  LARGE_INTEGER ft;
  // Convert to 100 nanosecond interval, negative value indicates relative time
  ft.QuadPart = -(10 * usec);
  timer = CreateWaitableTimer(NULL, TRUE, NULL);
  SetWaitableTimer(timer, &ft, 0, NULL, NULL, 0);
  WaitForSingleObject(timer, INFINITE);
  CloseHandle(timer);
}
