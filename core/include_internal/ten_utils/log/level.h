//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#pragma once

#include "ten_utils/ten_config.h"

#include "ten_utils/log/log.h"

/**
 * @brief When defined, ten_log library will not contain definition of global
 * output level variable. In that case it must be defined elsewhere using
 * TEN_LOG_DEFINE_GLOBAL_OUTPUT_LEVEL macro, for example:
 *
 *   TEN_LOG_DEFINE_GLOBAL_OUTPUT_LEVEL = TEN_LOG_WARN;
 *
 * This allows to specify custom value for static initialization and avoid
 * overhead of setting this value in runtime.
 */
#ifdef TEN_LOG_EXTERN_GLOBAL_OUTPUT_LEVEL
  #undef TEN_LOG_EXTERN_GLOBAL_OUTPUT_LEVEL
  #define TEN_LOG_EXTERN_GLOBAL_OUTPUT_LEVEL 1
#else
  #define TEN_LOG_EXTERN_GLOBAL_OUTPUT_LEVEL 0
#endif

TEN_UTILS_PRIVATE_API char ten_log_level_char(TEN_LOG_LEVEL level);
