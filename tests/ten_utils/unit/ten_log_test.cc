//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include "gtest/gtest.h"
#include "include_internal/ten_utils/log/level.h"
#include "include_internal/ten_utils/log/log.h"
#include "include_internal/ten_utils/log/output.h"

TEST(LogTest, FileOutput1) {  // NOLINT
  ten_log_t log;
  ten_log_init(&log);

  ten_log_set_output_level(&log, TEN_LOG_LEVEL_ERROR);
  ten_log_set_output_to_file(&log, "test1.log");

  TEN_LOGE_AUX(&log, "test %s test", "hello");

  ten_log_deinit(&log);
}
