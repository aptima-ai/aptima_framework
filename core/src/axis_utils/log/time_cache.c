//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_utils/axis_config.h"

#include "include_internal/axis_utils/log/time_cache.h"

#include <assert.h>
#include <stdbool.h>
#include <time.h>

#include "axis_utils/lib/atomic.h"

// There is no 'struct timeval' in Windows.
#if !defined(OS_WINDOWS)

// Indicates that the time cache is outdated.
#define TIME_CACHE_STALE (0x40000000)

// Indicate the time is updating.
#define TIME_CACHE_UPDATING (0x40000000 | 0x80000000)

// The initial value is `TIME_CACHE_STALE`, indicating that the cache is
// outdated.
static axis_atomic_t g_time_cache_mode = TIME_CACHE_STALE;

// Store the values of the time cache. g_time_cache_tv stores seconds and
// microseconds.
static struct timeval g_time_cache_tv = {0, 0};

// Store the values of the time cache. g_time_cache_tm stores the decomposed
// time structure.
static struct tm g_time_cache_tm;

bool axis_log_time_cache_get(const struct timeval *tv, struct tm *tm) {
  assert(tv && tm && "Invalid argument.");

  int64_t mode = axis_atomic_load(&g_time_cache_mode);
  if (0 == (mode & TIME_CACHE_UPDATING)) {
    // The time cache is up-to-date, so the reader can use 'tm' directly.
    //
    // And add 1 to indicate that there is one more 'reader', and this operation
    // will prevent the 'writer' to update the time.
    mode = axis_atomic_fetch_add(&g_time_cache_mode, 1);
    if (0 == (mode & TIME_CACHE_UPDATING)) {
      // The time cache is up-to-date, so the reader can use 'tm' directly.

      if (g_time_cache_tv.tv_sec == tv->tv_sec) {
        // Use the cached 'tm' directly.
        *tm = g_time_cache_tm;

        // Indicate that one 'reader' is completed.
        axis_atomic_sub_fetch(&g_time_cache_mode, 1);
        return true;
      }

      // Mark the time cache is outdated.
      axis_atomic_or_fetch(&g_time_cache_mode, TIME_CACHE_STALE);
    }

    // The time cache is _not_ up-to-date, so the reader can not use 'tm'
    // directly.
    axis_atomic_sub_fetch(&g_time_cache_mode, 1);
  }

  // The time cache is _not_ up-to-date, so the reader can not use 'tm'
  // directly.
  return false;
}

void axis_log_time_cache_set(const struct timeval *tv, struct tm *tm) {
  assert(tv && tm && "Invalid argument.");

  // Only update the time when the time cache is outdated.
  int64_t stale = TIME_CACHE_STALE;

  // Mark the time is updating, and create a critical section for the updating
  // operation.
  if (axis_atomic_bool_compare_swap(&g_time_cache_mode, stale,
                                   TIME_CACHE_UPDATING)) {
    g_time_cache_tv = *tv;
    g_time_cache_tm = *tm;

    // Mark the time cache is up-to-date.
    axis_atomic_and_fetch(&g_time_cache_mode, ~TIME_CACHE_UPDATING);
  }
}

#endif
