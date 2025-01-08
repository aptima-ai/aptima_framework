//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_utils/axis_config.h"

#include <assert.h>
#include <stdlib.h>

#include "include_internal/axis_utils/log/level.h"
#include "include_internal/axis_utils/log/log.h"
#include "include_internal/axis_utils/log/output.h"

axis_log_t axis_global_log = {axis_LOG_SIGNATURE,
                            axis_LOG_LEVEL_DEBUG,
                            {axis_log_output_to_stderr_cb, NULL, NULL}};

void axis_log_global_init(void) { axis_log_init(&axis_global_log); }

void axis_log_global_deinit(void) { axis_log_deinit(&axis_global_log); }

void axis_log_global_set_output_level(axis_LOG_LEVEL level) {
  axis_log_set_output_level(&axis_global_log, level);
}

void axis_log_global_set_output_to_stderr(void) {
  if (axis_log_is_output_to_file(&axis_global_log)) {
    axis_log_output_to_file_deinit(&axis_global_log);
  }
  axis_log_set_output_to_stderr(&axis_global_log);
}

void axis_log_global_set_output_to_file(const char *log_path) {
  axis_log_set_output_to_file(&axis_global_log, log_path);
}
