//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#pragma once

#include "ten_utils/ten_config.h"

#include "ten_utils/log/log.h"

#define TEN_LOG_OUT_STDERR \
  false, TEN_LOG_OUT_STDERR_MASK, ten_log_out_stderr_cb, NULL, NULL

/**
 * @brief Output to standard error stream. Library uses it by default, though in
 * few cases it could be necessary to specify it explicitly. For example, when
 * ten_log library is compiled with TEN_LOG_EXTERN_GLOBAL_OUTPUT, application
 * must define and initialize global output variable:
 *
 *   TEN_LOG_DEFINE_GLOBAL_OUTPUT = {TEN_LOG_OUT_STDERR};
 *
 * Another example is when using custom output, stderr could be used as a
 * fallback when custom output facility failed to initialize:
 *
 *   ten_log_set_output_v(TEN_LOG_OUT_STDERR);
 */
enum { TEN_LOG_OUT_STDERR_MASK = TEN_LOG_PUT_STD };

TEN_UTILS_PRIVATE_API void ten_log_out_stderr_cb(const ten_log_message_t *msg,
                                                 void *arg);

TEN_UTILS_PRIVATE_API void ten_log_out_file_cb(const ten_log_message_t *msg,
                                               void *arg);
