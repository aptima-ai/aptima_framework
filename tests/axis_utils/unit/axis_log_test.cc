//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "gtest/gtest.h"
#include "include_internal/aptima_utils/log/level.h"
#include "include_internal/aptima_utils/log/log.h"
#include "include_internal/aptima_utils/log/output.h"

TEST(LogTest, FileOutput1) {  // NOLINT
  aptima_log_t log;
  aptima_log_init(&log);

  aptima_log_set_output_level(&log, aptima_LOG_LEVEL_ERROR);
  aptima_log_set_output_to_file(&log, "test1.log");

  aptima_LOGE_AUX(&log, "test %s test", "hello");

  aptima_log_deinit(&log);
}
