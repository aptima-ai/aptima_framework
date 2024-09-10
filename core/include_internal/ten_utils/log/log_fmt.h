//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#pragma once

#include "ten_utils/ten_config.h"

/**
 * @brief When defined, ten_log library will not contain definition of global
 * format variable. In that case it must be defined elsewhere using
 * TEN_LOG_DEFINE_GLOBAL_FORMAT macro, for example:
 *
 *   TEN_LOG_DEFINE_GLOBAL_FORMAT = {MEM_WIDTH};
 *
 * This allows to specify custom value for static initialization and avoid
 * overhead of setting this value in runtime.
 */
#ifdef TEN_LOG_EXTERN_GLOBAL_FORMAT
  #undef TEN_LOG_EXTERN_GLOBAL_FORMAT
  #define TEN_LOG_EXTERN_GLOBAL_FORMAT 1
#else
  #define TEN_LOG_EXTERN_GLOBAL_FORMAT 0
#endif

/**
 * @brief Default number of bytes in one line of memory output. For large values
 * TEN_LOG_BUF_SZ also must be increased.
 */
#ifndef TEN_LOG_MEM_WIDTH
  #define TEN_LOG_MEM_WIDTH 32
#endif
