//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include "ten_utils/ten_config.h"

#include "include_internal/ten_utils/log/time.h"

#include <assert.h>
#include <time.h>

#include "include_internal/ten_utils/log/format.h"
#include "include_internal/ten_utils/log/time_cache.h"

#if defined(_WIN32) || defined(_WIN64)
  #include <windows.h>
#else

  #include <sys/time.h>
  #include <unistd.h>

  #if defined(__linux__)
    #include <linux/limits.h>
  #elif defined(__MACH__)
    #include <sys/syslimits.h>
  #endif

#endif

ten_log_get_time_func_t g_ten_log_get_time = ten_log_get_time;

void ten_log_get_time(struct tm *tm, size_t *msec) {
  assert(tm && msec && "Invalid argument.");

#if !_TEN_LOG_MESSAGE_FORMAT_DATETIME_USED
  VAR_UNUSED(tm);
  VAR_UNUSED(msec);
#else

  #if defined(_WIN32) || defined(_WIN64)
  SYSTEMTIME st;
  GetLocalTime(&st);
  tm->tm_year = st.wYear;
  tm->tm_mon = st.wMonth - 1;
  tm->tm_mday = st.wDay;
  tm->tm_wday = st.wDayOfWeek;
  tm->tm_hour = st.wHour;
  tm->tm_min = st.wMinute;
  tm->tm_sec = st.wSecond;
  *msec = st.wMilliseconds;
  #else
  struct timeval tv;
  gettimeofday(&tv, 0);

  if (!ten_log_time_cache_get(&tv, tm)) {
    localtime_r(&tv.tv_sec, tm);
    ten_log_time_cache_set(&tv, tm);
  }

  *msec = (unsigned)tv.tv_usec / 1000;
  #endif

#endif
}
