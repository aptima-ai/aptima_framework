//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "ten_utils/ten_config.h"

#include "include_internal/ten_utils/log/time.h"

#include <assert.h>
#include <time.h>

#include "include_internal/ten_utils/log/time_cache.h"
#include "ten_utils/lib/string.h"

#if defined(OS_WINDOWS)
#include <windows.h>
#else

#include <sys/time.h>
#include <unistd.h>

#if defined(OS_LINUX)
#include <linux/limits.h>
#elif defined(OS_MACOS)
#include <sys/syslimits.h>
#endif

#endif

// Maximum length of "MM-DD HH:MM:SS.XXX"
//                     212 1 212 12 1 3 = 18
#define TIME_LOG_SIZE (size_t)(18 * 2)

void ten_log_get_time(struct tm *time_info, size_t *msec) {
  assert(time_info && msec && "Invalid argument.");

#if defined(OS_WINDOWS)
  SYSTEMTIME st;
  GetLocalTime(&st);
  time_info->tm_year = st.wYear;
  time_info->tm_mon = st.wMonth - 1;
  time_info->tm_mday = st.wDay;
  time_info->tm_wday = st.wDayOfWeek;
  time_info->tm_hour = st.wHour;
  time_info->tm_min = st.wMinute;
  time_info->tm_sec = st.wSecond;
  *msec = st.wMilliseconds;
#else
  struct timeval tv;
  gettimeofday(&tv, 0);

  if (!ten_log_time_cache_get(&tv, time_info)) {
    (void)localtime_r(&tv.tv_sec, time_info);
    ten_log_time_cache_set(&tv, time_info);
  }

  *msec = (unsigned)tv.tv_usec / 1000;
#endif
}

void ten_log_add_time_string(ten_string_t *buf, struct tm *time_info,
                             size_t msec) {
  assert(buf && "Invalid argument.");

  ten_string_reserve(buf, TIME_LOG_SIZE);

  // Format the date and time into the buffer, excluding milliseconds.
  size_t written = strftime(&buf->buf[buf->first_unused_idx],
                            buf->buf_size - buf->first_unused_idx,
                            "%m-%d %H:%M:%S", time_info);
  assert(written && "Should not happen.");

  buf->first_unused_idx = strlen(buf->buf);

  ten_string_append_formatted(buf, ".%03zu", msec);
}
