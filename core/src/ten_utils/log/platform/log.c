//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include "ten_utils/ten_config.h"

/**
 * @brief When defined, ten_log library will not contain definition of global
 * output variable. In that case it must be defined elsewhere using
 * TEN_LOG_DEFINE_GLOBAL_OUTPUT macro, for example:
 *
 *   TEN_LOG_DEFINE_GLOBAL_OUTPUT = {TEN_LOG_PUT_STD, custom_output_callback};
 *
 * This allows to specify custom value for static initialization and avoid
 * overhead of setting this value in runtime.
 */
#ifdef TEN_LOG_EXTERN_GLOBAL_OUTPUT
  #undef TEN_LOG_EXTERN_GLOBAL_OUTPUT
  #define TEN_LOG_EXTERN_GLOBAL_OUTPUT 1
#else
  #define TEN_LOG_EXTERN_GLOBAL_OUTPUT 0
#endif
