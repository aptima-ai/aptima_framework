//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#pragma once

#include "ten_utils/ten_config.h"

#include <assert.h>

/**
 * @brief String to put in the end of each log line (can be empty). Its value
 * used by stderr output callback. Its size used as a default value for
 * TEN_LOG_EOL_SZ.
 */
#ifndef TEN_LOG_EOL
  #define TEN_LOG_EOL "\n"
#endif

/**
 * @brief Number of bytes to reserve for EOL in the log line buffer (must be
 * >0). Must be larger than or equal to length of TEN_LOG_EOL with terminating
 * null.
 */
#ifndef TEN_LOG_EOL_SZ
  #define TEN_LOG_EOL_SZ sizeof(TEN_LOG_EOL)
#endif
