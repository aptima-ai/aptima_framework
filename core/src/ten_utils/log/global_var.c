//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include "ten_utils/ten_config.h"

#include "include_internal/ten_utils/log/log_fmt.h"
#include "include_internal/ten_utils/log/platform/general/log.h"
#include "include_internal/ten_utils/log/platform/mac/log.h"
#include "include_internal/ten_utils/log/platform/win/log.h"
#include "include_internal/ten_utils/log/tag.h"
#include "ten_utils/log/log.h"

#if !TEN_LOG_EXTERN_TAG_PREFIX
// Define the variable for tag_prefix.
TEN_LOG_DEFINE_TAG_PREFIX = NULL;
#endif

#if !TEN_LOG_EXTERN_GLOBAL_FORMAT
// Define the variable for global_format.
TEN_LOG_DEFINE_GLOBAL_FORMAT = {false, TEN_LOG_MEM_WIDTH};
#endif

#if !TEN_LOG_EXTERN_GLOBAL_OUTPUT_LEVEL
// Define the variable for global_output_level.
TEN_LOG_DEFINE_GLOBAL_OUTPUT_LEVEL = TEN_LOG_NONE;
#endif

#if !TEN_LOG_EXTERN_GLOBAL_OUTPUT
  #if TEN_LOG_USE_ANDROID_LOG
TEN_LOG_DEFINE_GLOBAL_OUTPUT = {OUT_ANDROID};
  #elif TEN_LOG_USE_NSLOG
TEN_LOG_DEFINE_GLOBAL_OUTPUT = {OUT_NSLOG};
  #elif TEN_LOG_USE_DEBUGSTRING
TEN_LOG_DEFINE_GLOBAL_OUTPUT = {OUT_DEBUGSTRING};
  #else
TEN_LOG_DEFINE_GLOBAL_OUTPUT = {TEN_LOG_OUT_STDERR};
  #endif
#endif

static ten_log_output_t out_stderr = {TEN_LOG_OUT_STDERR};

const ten_log_t ten_log_stderr_spec = {
    TEN_LOG_SIGNATURE,
    TEN_LOG_GLOBAL_FORMAT,
    &out_stderr,
};

const ten_log_t global_spec = {
    TEN_LOG_SIGNATURE,
    TEN_LOG_GLOBAL_FORMAT,
    TEN_LOG_GLOBAL_OUTPUT,
};
