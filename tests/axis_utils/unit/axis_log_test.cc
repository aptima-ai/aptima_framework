//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "gtest/gtest.h"
#include "include_internal/axis_utils/log/level.h"
#include "include_internal/axis_utils/log/log.h"
#include "include_internal/axis_utils/log/output.h"

TEST(LogTest, FileOutput1) {  // NOLINT
  axis_log_t log;
  axis_log_init(&log);

  axis_log_set_output_level(&log, axis_LOG_LEVEL_ERROR);
  axis_log_set_output_to_file(&log, "test1.log");

  axis_LOGE_AUX(&log, "test %s test", "hello");

  axis_log_deinit(&log);
}
