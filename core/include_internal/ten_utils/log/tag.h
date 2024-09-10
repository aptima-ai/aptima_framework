//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#pragma once

#include "ten_utils/ten_config.h"

/**
 * @brief When defined, ten_log library will not contain definition of tag
 * prefix variable. In that case it must be defined elsewhere using
 * TEN_LOG_DEFINE_TAG_PREFIX macro, for example:
 *
 *   TEN_LOG_DEFINE_TAG_PREFIX = "ProcessName";
 *
 * This allows to specify custom value for static initialization and avoid
 * overhead of setting this value in runtime.
 */
#ifdef TEN_LOG_EXTERN_TAG_PREFIX
  #undef TEN_LOG_EXTERN_TAG_PREFIX
  #define TEN_LOG_EXTERN_TAG_PREFIX 1
#else
  #define TEN_LOG_EXTERN_TAG_PREFIX 0
#endif
